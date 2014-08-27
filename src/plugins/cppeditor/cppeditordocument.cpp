/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cppeditordocument.h"

#include "cppeditorconstants.h"
#include "cpphighlighter.h"

#include <cpptools/builtineditordocumentprocessor.h>
#include <cpptools/cppcodeformatter.h>
#include <cpptools/cppcodemodelsettings.h>
#include <cpptools/cppmodelmanagerinterface.h>
#include <cpptools/cppqtstyleindenter.h>
#include <cpptools/cpptoolsconstants.h>
#include <cpptools/cpptoolsplugin.h>

#include <utils/qtcassert.h>
#include <utils/runextensions.h>

#include <QTextDocument>

namespace {

CppTools::CppModelManagerInterface *mm()
{
    return CppTools::CppModelManagerInterface::instance();
}

} // anonymous namespace

namespace CppEditor {
namespace Internal {

enum { processDocumentIntervalInMs = 150 };

class CppEditorDocumentHandle : public CppTools::EditorDocumentHandle
{
public:
    CppEditorDocumentHandle(CppEditor::Internal::CppEditorDocument *cppEditorDocument)
        : m_cppEditorDocument(cppEditorDocument)
        , m_registrationFilePath(cppEditorDocument->filePath())
    {
        mm()->registerEditorDocument(this);
    }

    ~CppEditorDocumentHandle() { mm()->unregisterEditorDocument(m_registrationFilePath); }

    QString filePath() const { return m_cppEditorDocument->filePath(); }
    QByteArray contents() const { return m_cppEditorDocument->contentsText(); }
    unsigned revision() const { return m_cppEditorDocument->contentsRevision(); }

    CppTools::BaseEditorDocumentProcessor *processor()
    { return m_cppEditorDocument->processor(); }

private:
    CppEditor::Internal::CppEditorDocument * const m_cppEditorDocument;
    // The file path of the editor document can change (e.g. by "Save As..."), so make sure
    // that un-registration happens with the path the document was registered.
    const QString m_registrationFilePath;
};

CppEditorDocument::CppEditorDocument()
    : m_fileIsBeingReloaded(false)
    , m_isObjCEnabled(false)
    , m_cachedContentsRevision(-1)
    , m_processorRevision(0)
    , m_completionAssistProvider(0)
{
    setId(CppEditor::Constants::CPPEDITOR_ID);
    setSyntaxHighlighter(new CppHighlighter);
    setIndenter(new CppTools::CppQtStyleIndenter);

    connect(this, SIGNAL(tabSettingsChanged()), this, SLOT(invalidateFormatterCache()));
    connect(this, SIGNAL(mimeTypeChanged()), this, SLOT(onMimeTypeChanged()));

    connect(this, SIGNAL(aboutToReload()), this, SLOT(onAboutToReload()));
    connect(this, SIGNAL(reloadFinished(bool)), this, SLOT(onReloadFinished()));
    connect(this, SIGNAL(filePathChanged(QString,QString)),
            this, SLOT(onFilePathChanged(QString,QString)));

    m_processorTimer.setSingleShot(true);
    m_processorTimer.setInterval(processDocumentIntervalInMs);
    connect(&m_processorTimer, SIGNAL(timeout()), this, SLOT(processDocument()));

    // See also onFilePathChanged() for more initialization
}

CppEditorDocument::~CppEditorDocument()
{
}

bool CppEditorDocument::isObjCEnabled() const
{
    return m_isObjCEnabled;
}

CppTools::CppCompletionAssistProvider *CppEditorDocument::completionAssistProvider() const
{
    return m_completionAssistProvider;
}

void CppEditorDocument::semanticRehighlight()
{
    CppTools::BaseEditorDocumentProcessor *p = processor();
    QTC_ASSERT(p, return);
    p->semanticRehighlight(true);
}

CppTools::SemanticInfo CppEditorDocument::recalculateSemanticInfo()
{
    CppTools::BaseEditorDocumentProcessor *p = processor();
    QTC_ASSERT(p, CppTools::SemanticInfo());
    return p->recalculateSemanticInfo();
}

QByteArray CppEditorDocument::contentsText() const
{
    QMutexLocker locker(&m_cachedContentsLock);

    const int currentRevision = document()->revision();
    if (m_cachedContentsRevision != currentRevision && !m_fileIsBeingReloaded) {
        m_cachedContentsRevision = currentRevision;
        m_cachedContents = plainText().toUtf8();
    }

    return m_cachedContents;
}

void CppEditorDocument::applyFontSettings()
{
    if (TextEditor::SyntaxHighlighter *highlighter = syntaxHighlighter()) {
        // Clear all additional formats since they may have changed
        QTextBlock b = document()->firstBlock();
        while (b.isValid()) {
            QList<QTextLayout::FormatRange> noFormats;
            highlighter->setExtraAdditionalFormats(b, noFormats);
            b = b.next();
        }
    }
    BaseTextDocument::applyFontSettings(); // rehighlights and updates additional formats
}

void CppEditorDocument::invalidateFormatterCache()
{
    CppTools::QtStyleCodeFormatter formatter;
    formatter.invalidateCache(document());
}

void CppEditorDocument::onMimeTypeChanged()
{
    const QString &mt = mimeType();
    m_isObjCEnabled = (mt == QLatin1String(CppTools::Constants::OBJECTIVE_C_SOURCE_MIMETYPE)
                       || mt == QLatin1String(CppTools::Constants::OBJECTIVE_CPP_SOURCE_MIMETYPE));
    m_completionAssistProvider = mm()->completionAssistProvider(mt);
}

void CppEditorDocument::onAboutToReload()
{
    QTC_CHECK(!m_fileIsBeingReloaded);
    m_fileIsBeingReloaded = true;
}

void CppEditorDocument::onReloadFinished()
{
    QTC_CHECK(m_fileIsBeingReloaded);
    m_fileIsBeingReloaded = false;
}

void CppEditorDocument::onFilePathChanged(const QString &oldPath, const QString &newPath)
{
    Q_UNUSED(oldPath);

    if (!newPath.isEmpty()) {
        setMimeType(Core::MimeDatabase::findByFile(QFileInfo(newPath)).type());

        disconnect(this, SIGNAL(contentsChanged()), this, SLOT(scheduleProcessDocument()));
        connect(this, SIGNAL(contentsChanged()), this, SLOT(scheduleProcessDocument()));

        // Un-Register/Register in ModelManager
        m_editorDocumentHandle.reset(new CppEditorDocumentHandle(this));

        resetProcessor();
        m_processorRevision = document()->revision();
        processDocument();
    }
}

void CppEditorDocument::scheduleProcessDocument()
{
    m_processorRevision = document()->revision();
    m_processorTimer.start(processDocumentIntervalInMs);
}

void CppEditorDocument::processDocument()
{
    if (processor()->isParserRunning() || m_processorRevision != contentsRevision()) {
        m_processorTimer.start();
        return;
    }

    m_processorTimer.stop();
    if (m_fileIsBeingReloaded || filePath().isEmpty())
        return;

    processor()->run();
}

void CppEditorDocument::resetProcessor()
{
    releaseResources();
    processor(); // creates a new processor
}

unsigned CppEditorDocument::contentsRevision() const
{
    return document()->revision();
}

void CppEditorDocument::releaseResources()
{
    if (m_processor)
        disconnect(m_processor.data(), 0, this, 0);
    m_processor.reset();
}

CppTools::BaseEditorDocumentProcessor *CppEditorDocument::processor()
{
    if (!m_processor) {
        m_processor.reset(mm()->editorDocumentProcessor(this));
        connect(m_processor.data(), &CppTools::BaseEditorDocumentProcessor::codeWarningsUpdated,
                this, &CppEditorDocument::codeWarningsUpdated);
        connect(m_processor.data(), &CppTools::BaseEditorDocumentProcessor::ifdefedOutBlocksUpdated,
                this, &CppEditorDocument::ifdefedOutBlocksUpdated);
        connect(m_processor.data(), &CppTools::BaseEditorDocumentProcessor::cppDocumentUpdated,
                this, &CppEditorDocument::cppDocumentUpdated);
        connect(m_processor.data(), &CppTools::BaseEditorDocumentProcessor::semanticInfoUpdated,
                this, &CppEditorDocument::semanticInfoUpdated);
    }

    return m_processor.data();
}

} // namespace Internal
} // namespace CppEditor
