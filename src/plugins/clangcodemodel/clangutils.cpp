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
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "clangutils.h"

#include <clang-c/Index.h>

#include <coreplugin/documentmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>

#include <cpptools/cppprojects.h>
#include <cpptools/cppworkingcopy.h>

#include <QDir>
#include <QFile>
#include <QSet>
#include <QString>

using namespace ClangCodeModel;
using namespace ClangCodeModel::Internal;
using namespace Core;
using namespace CppTools;

static const bool BeVerbose = qgetenv("QTC_CLANG_VERBOSE") == "1";

namespace ClangCodeModel {
namespace Utils {

namespace {
bool isBlacklisted(const QString &path)
{
    static QStringList blacklistedPaths = QStringList()
            << QLatin1String("lib/gcc/i686-apple-darwin");

    foreach (const QString &blacklisted, blacklistedPaths)
        if (path.contains(blacklisted))
            return true;

    return false;
}
} // anonymous namespace

UnsavedFiles createUnsavedFiles(WorkingCopy workingCopy)
{
    // TODO: change the modelmanager to hold one working copy, and amend it every time we ask for one.
    // TODO: Reason: the UnsavedFile needs a QByteArray.

    QSet<QString> modifiedFiles;
    foreach (IDocument *doc, Core::DocumentManager::modifiedDocuments())
        modifiedFiles.insert(doc->filePath());

    UnsavedFiles result;
    QHashIterator<QString, QPair<QByteArray, unsigned> > wcIter = workingCopy.iterator();
    while (wcIter.hasNext()) {
        wcIter.next();
        const QString &fileName = wcIter.key();
        if (modifiedFiles.contains(fileName) && QFile(fileName).exists())
            result.insert(fileName, wcIter.value().first);
    }

    return result;
}

/**
 * @brief Creates list of command-line arguments required for correct parsing
 * @param pPart Null if file isn't part of any project
 * @param fileName Path to file, non-empty
 */
QStringList createClangOptions(const ProjectPart::Ptr &pPart, const QString &fileName)
{
    ProjectFile::Kind fileKind = ProjectFile::Unclassified;
    if (!pPart.isNull())
        foreach (const ProjectFile &file, pPart->files)
            if (file.path == fileName) {
                fileKind = file.kind;
                break;
            }
    if (fileKind == ProjectFile::Unclassified)
        fileKind = ProjectFile::classify(fileName);

    return createClangOptions(pPart, fileKind);
}

static QString getResourceDir()
{
    QDir dir(Core::ICore::instance()->resourcePath() + QLatin1String("/cplusplus/clang/") +
             QLatin1String(CLANG_VERSION) + QLatin1String("/include"));
    if (!dir.exists() || !QFileInfo(dir, QLatin1String("stdint.h")).exists())
        dir = QDir(QLatin1String(CLANG_RESOURCE_DIR));
    return dir.canonicalPath();
}

static bool maybeIncludeBorlandExtensions()
{
    return
#if defined(CINDEX_VERSION) // clang 3.2 or higher
        true;
#else
        false;
#endif
}

/**
 * @brief Creates list of command-line arguments required for correct parsing
 * @param pPart Null if file isn't part of any project
 * @param fileKind Determines language and source/header state
 */
QStringList createClangOptions(const ProjectPart::Ptr &pPart, ProjectFile::Kind fileKind)
{
    QStringList result;
    if (pPart.isNull())
        return result;

    if (BeVerbose)
        result << QLatin1String("-v");

    const bool objcExt = pPart->languageExtensions & ProjectPart::ObjectiveCExtensions;
    result << CppTools::CompilerOptionsBuilder::createLanguageOption(fileKind, objcExt);
    result << CppTools::CompilerOptionsBuilder::createOptionsForLanguage(
                                                      pPart->languageVersion,
                                                      pPart->languageExtensions,
                                                      maybeIncludeBorlandExtensions());
    result << CppTools::CompilerOptionsBuilder::createDefineOptions(pPart->toolchainDefines);
    result << CppTools::CompilerOptionsBuilder::createDefineOptions(pPart->projectDefines);
    result << CppTools::CompilerOptionsBuilder::createHeaderPathOptions(pPart->headerPaths,
                                                                        isBlacklisted);

    // Inject header file
    static const QString injectedHeader = Core::ICore::instance()->resourcePath()
            + QLatin1String("/cplusplus/qt%1-qobjectdefs-injected.h");
//    if (qtVersion == ProjectPart::Qt4)
//        opts << QLatin1String("-include") << injectedHeader.arg(QLatin1Char('4'));
    if (pPart->qtVersion == ProjectPart::Qt5)
        result << QLatin1String("-include") << injectedHeader.arg(QLatin1Char('5'));

    if (!pPart->projectConfigFile.isEmpty())
        result << QLatin1String("-include") << pPart->projectConfigFile;

    static const QString resourceDir = getResourceDir();
    if (!resourceDir.isEmpty()) {
        result << QLatin1String("-nostdlibinc");
        result << (QLatin1String("-I") + resourceDir);
        result << QLatin1String("-undef");
    }

    result << QLatin1String("-fmessage-length=0");
    result << QLatin1String("-fdiagnostics-show-note-include-stack");
    result << QLatin1String("-fmacro-backtrace-limit=0");
    result << QLatin1String("-fretain-comments-from-system-headers");
    // TODO: -Xclang -ferror-limit -Xclang 0 ?

    return result;
}

/// @return Option to speed up parsing with precompiled header
QStringList createPCHInclusionOptions(const QStringList &pchFiles)
{
    QStringList opts;

    foreach (const QString &pchFile, pchFiles) {
        if (QFile(pchFile).exists()) {
            opts += QLatin1String("-include-pch");
            opts += pchFile;
        }
    }

    return opts;
}

/// @return Option to speed up parsing with precompiled header
QStringList createPCHInclusionOptions(const QString &pchFile)
{
    return createPCHInclusionOptions(QStringList() << pchFile);
}

} // namespace Utils
} // namespace Clang
