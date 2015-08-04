/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cppeditordocument.h"

#include "cppeditorconstants.h"
#include "cpphighlighter.h"

#include <cpptools/baseeditordocumentparser.h>
#include <cpptools/builtineditordocumentprocessor.h>
#include <cpptools/cppcodeformatter.h>
#include <cpptools/cppcodemodelsettings.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/cppqtstyleindenter.h>
#include <cpptools/cpptoolsconstants.h>
#include <cpptools/cpptoolsplugin.h>

#include <projectexplorer/session.h>

#include <coreplugin/editormanager/editormanager.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>

#include <QTextDocument>

namespace {

CppTools::CppModelManager *mm()
{
    return CppTools::CppModelManager::instance();
}

} // anonymous namespace

namespace CppEditor {
namespace Internal {

enum { processDocumentIntervalInMs = 150 };

class CppEditorDocumentHandleImpl : public CppTools::CppEditorDocumentHandle
{
public:
    CppEditorDocumentHandleImpl(CppEditorDocument *cppEditorDocument)
        : m_cppEditorDocument(cppEditorDocument)
        , m_registrationFilePath(cppEditorDocument->filePath().toString())
    {
        mm()->registerCppEditorDocument(this);
    }

    ~CppEditorDocumentHandleImpl() { mm()->unregisterCppEditorDocument(m_registrationFilePath); }

    QString filePath() const override { return m_cppEditorDocument->filePath().toString(); }
    QByteArray contents() const override { return m_cppEditorDocument->contentsText(); }
    unsigned revision() const override { return m_cppEditorDocument->contentsRevision(); }

    CppTools::BaseEditorDocumentProcessor *processor() const override
    { return m_cppEditorDocument->processor(); }

    void resetProcessor()
    { m_cppEditorDocument->resetProcessor(); }

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
    connect(this, &IDocument::filePathChanged,
            this, &CppEditorDocument::onFilePathChanged);

    m_processorTimer.setSingleShot(true);
    m_processorTimer.setInterval(processDocumentIntervalInMs);
    connect(&m_processorTimer, SIGNAL(timeout()), this, SLOT(processDocument()));

    // See also onFilePathChanged() for more initialization
}

bool CppEditorDocument::isObjCEnabled() const
{
    return m_isObjCEnabled;
}

TextEditor::CompletionAssistProvider *CppEditorDocument::completionAssistProvider() const
{
    return m_completionAssistProvider;
}

void CppEditorDocument::recalculateSemanticInfoDetached()
{
    CppTools::BaseEditorDocumentProcessor *p = processor();
    QTC_ASSERT(p, return);
    p->recalculateSemanticInfoDetached(true);
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
    TextDocument::applyFontSettings(); // rehighlights and updates additional formats
    if (m_processor)
        m_processor->semanticRehighlight();
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

void CppEditorDocument::onFilePathChanged(const Utils::FileName &oldPath,
                                          const Utils::FileName &newPath)
{
    Q_UNUSED(oldPath);

    if (!newPath.isEmpty()) {
        Utils::MimeDatabase mdb;
        setMimeType(mdb.mimeTypeForFile(newPath.toFileInfo()).name());

        disconnect(this, SIGNAL(contentsChanged()), this, SLOT(scheduleProcessDocument()));
        connect(this, SIGNAL(contentsChanged()), this, SLOT(scheduleProcessDocument()));

        // Un-Register/Register in ModelManager
        m_editorDocumentHandle.reset();
        m_editorDocumentHandle.reset(new CppEditorDocumentHandleImpl(this));

        resetProcessor();
        updatePreprocessorSettings();
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

void CppEditorDocument::updatePreprocessorSettings()
{
    if (filePath().isEmpty())
        return;

    const QString prefix = QLatin1String(Constants::CPP_PREPROCESSOR_PROJECT_PREFIX);
    const QString &projectPartId = ProjectExplorer::SessionManager::value(
                prefix + filePath().toString()).toString();
    const QString directivesKey = projectPartId + QLatin1Char(',') + filePath().toString();
    const QByteArray additionalDirectives = ProjectExplorer::SessionManager::value(
                directivesKey).toString().toUtf8();

    setPreprocessorSettings(mm()->projectPartForId(projectPartId), additionalDirectives);
}

void CppEditorDocument::setPreprocessorSettings(const CppTools::ProjectPart::Ptr &projectPart,
                                                const QByteArray &defines)
{
    CppTools::BaseEditorDocumentParser *parser = processor()->parser();
    QTC_ASSERT(parser, return);
    if (parser->projectPart() != projectPart || parser->configuration().editorDefines != defines) {
        CppTools::BaseEditorDocumentParser::Configuration config = parser->configuration();
        config.manuallySetProjectPart = projectPart;
        config.editorDefines = defines;
        parser->setConfiguration(config);

        emit preprocessorSettingsChanged(!defines.trimmed().isEmpty());
    }
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
