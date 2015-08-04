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

#include "clangeditordocumentparser.h"
#include "clangutils.h"
#include "pchinfo.h"
#include "pchmanager.h"

#include <cpptools/cppmodelmanager.h>
#include <cpptools/cppprojects.h>
#include <cpptools/cppworkingcopy.h>

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QLoggingCategory>

static Q_LOGGING_CATEGORY(log, "qtc.clangcodemodel.clangeditordocumentparser")

namespace {

QStringList createOptions(const QString &filePath,
                          const CppTools::ProjectPart::Ptr &part,
                          bool includeSpellCheck = false)
{
    using namespace ClangCodeModel;

    QStringList options;
    if (part.isNull())
        return options;

    if (includeSpellCheck)
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
    BaseEditorDocumentParser::Configuration config = configuration();
    config.stickToPreviousProjectPart = false;
    setConfiguration(config);
}

void ClangEditorDocumentParser::updateHelper(const BaseEditorDocumentParser::InMemoryInfo &info)
{
    QTC_ASSERT(m_marker, return);

    // Determine project part
    State state_ = state();
    state_.projectPart = determineProjectPart(filePath(), configuration(), state_);
    setState(state_);

    // Determine command line arguments
    const QStringList options = createOptions(filePath(), state_.projectPart, true);
    qCDebug(log, "Reparse options (cmd line equivalent): %s",
           commandLine(options, filePath()).toUtf8().constData());

    // Run
    QTime t; t.start();
    QMutexLocker lock(m_marker->mutex());
    m_marker->setFileName(filePath());
    m_marker->setCompilationOptions(options);
    const Internal::UnsavedFiles unsavedFiles = Utils::createUnsavedFiles(info.workingCopy,
                                                                          info.modifiedFiles);
    m_marker->reparse(unsavedFiles);
    qCDebug(log) << "Reparse took" << t.elapsed() << "ms.";
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
