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

#include "clangeditordocumentparser.h"
#include "clangutils.h"
#include "pchinfo.h"
#include "pchmanager.h"

#include <cpptools/cppmodelmanagerinterface.h>
#include <cpptools/cppprojects.h>
#include <cpptools/cppworkingcopy.h>

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

static const bool DebugTiming = qgetenv("QTC_CLANG_VERBOSE") == "1";

namespace {

QStringList createOptions(const QString &filePath, const CppTools::ProjectPart::Ptr &part)
{
    using namespace ClangCodeModel;

    QStringList options;
    if (part.isNull())
        return options;

    options += QLatin1String("-fspell-checking");
    options += ClangCodeModel::Utils::createClangOptions(part, filePath);

    if (Internal::PchInfo::Ptr pchInfo = Internal::PchManager::instance()->pchInfo(part))
        options.append(ClangCodeModel::Utils::createPCHInclusionOptions(pchInfo->fileName()));

    return options;
}

QString commandLine(const QStringList &options, const QString &fileName)
{
    const QStringList allOptions = QStringList(options)
        << QLatin1String("-fsyntax-only") << fileName;
    QStringList allOptionsQuoted;
    foreach (const QString &option, allOptions)
        allOptionsQuoted.append(QLatin1Char('\'') + option + QLatin1Char('\''));
    return ::Utils::HostOsInfo::withExecutableSuffix(QLatin1String("clang"))
        + QLatin1Char(' ') + allOptionsQuoted.join(QLatin1Char(' '));
}

} // anonymous namespace

namespace ClangCodeModel {

ClangEditorDocumentParser::ClangEditorDocumentParser(const QString &filePath)
    : BaseEditorDocumentParser(filePath)
    , m_marker(new ClangCodeModel::SemanticMarker)
{
}

void ClangEditorDocumentParser::update(CppTools::WorkingCopy workingCopy)
{
    QTC_ASSERT(m_marker, return);
    QMutexLocker lock(m_marker->mutex());
    QMutexLocker lock2(&m_mutex);

    updateProjectPart();
    const QStringList options = createOptions(filePath(), projectPart());

    QTime t;
    if (DebugTiming) {
        qDebug("*** Reparse options (cmd line equivalent): %s",
               commandLine(options, filePath()).toUtf8().constData());
        t.start();
    }

    m_marker->setFileName(filePath());
    m_marker->setCompilationOptions(options);
    const Internal::UnsavedFiles unsavedFiles = Utils::createUnsavedFiles(workingCopy);
    m_marker->reparse(unsavedFiles);

    if (DebugTiming)
        qDebug() << "*** Reparse took" << t.elapsed() << "ms.";
}

QList<Diagnostic> ClangEditorDocumentParser::diagnostics() const
{
    QTC_ASSERT(m_marker, return QList<Diagnostic>());
    QMutexLocker(m_marker->mutex());
    return m_marker->diagnostics();
}

QList<SemanticMarker::Range> ClangEditorDocumentParser::ifdefedOutBlocks() const
{
    QTC_ASSERT(m_marker, return QList<SemanticMarker::Range>());
    QMutexLocker(m_marker->mutex());
    return m_marker->ifdefedOutBlocks();
}

SemanticMarker::Ptr ClangEditorDocumentParser::semanticMarker() const
{
    return m_marker;
}

} // namespace ClangCodeModel
