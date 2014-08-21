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

#include "clangutils.h"

#include <clang-c/Index.h>

#include <coreplugin/documentmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>

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

static QStringList buildDefines(const QByteArray &defines, bool toolchainDefines)
{
    QStringList result;

    foreach (QByteArray def, defines.split('\n')) {
        if (def.isEmpty())
            continue;

        // This is a quick fix for QTCREATORBUG-11501.
        // TODO: do a proper fix, see QTCREATORBUG-11709.
        if (def.startsWith("#define __cplusplus"))
            continue;

        // TODO: verify if we can pass compiler-defined macros when also passing -undef.
        if (toolchainDefines) {
            //### FIXME: the next 3 check shouldn't be needed: we probably don't want to get the compiler-defined defines in.
            if (!def.startsWith("#define "))
                continue;
            if (def.startsWith("#define _"))
                continue;
            if (def.startsWith("#define OBJC_NEW_PROPERTIES"))
                continue;
        }

        QByteArray str = def.mid(8);
        int spaceIdx = str.indexOf(' ');
        QString arg;
        if (spaceIdx != -1) {
            arg = QLatin1String("-D" + str.left(spaceIdx) + "=" + str.mid(spaceIdx + 1));
        } else {
            arg = QLatin1String("-D" + str);
        }
        arg = arg.replace(QLatin1String("\\\""), QLatin1String("\""));
        arg = arg.replace(QLatin1String("\""), QLatin1String(""));
        if (!result.contains(arg))
            result.append(arg);
    }

    return result;
}

static QString getResourceDir()
{
    QDir dir(Core::ICore::instance()->resourcePath() + QLatin1String("/cplusplus/clang/") +
             QLatin1String(CLANG_VERSION) + QLatin1String("/include"));
    if (!dir.exists() || !QFileInfo(dir, QLatin1String("stdint.h")).exists())
        dir = QDir(QLatin1String(CLANG_RESOURCE_DIR));
    return dir.canonicalPath();
}

/**
 * @brief Creates list of command-line arguments required for correct parsing
 * @param pPart Null if file isn't part of any project
 * @param fileKind Determines language and source/header state
 */
QStringList createClangOptions(const ProjectPart::Ptr &pPart, ProjectFile::Kind fileKind)
{
    const bool objcExt = pPart->languageExtensions & ProjectPart::ObjectiveCExtensions;
    QStringList result = clangLanguageOption(fileKind, objcExt);
    result << clangOptionsForLanguage(pPart->qtVersion,
                                      pPart->languageVersion,
                                      pPart->languageExtensions);

    if (pPart.isNull())
        return result;

    static const QString resourceDir = getResourceDir();

    if (BeVerbose)
        result << QLatin1String("-v");

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

    if (!pPart->projectConfigFile.isEmpty())
        result << QLatin1String("-include") << pPart->projectConfigFile;

    result << buildDefines(pPart->toolchainDefines, false);
    result << buildDefines(pPart->projectDefines, false);

    typedef ProjectPart::HeaderPath HeaderPath;
    foreach (const HeaderPath &headerPath , pPart->headerPaths) {
        if (headerPath.path.isEmpty() || isBlacklisted(headerPath.path))
            continue;

        QString prefix;
        switch (headerPath.type) {
        case HeaderPath::FrameworkPath:
            prefix = QLatin1String("-F");
            break;
        default: // This shouldn't happen, but let's be nice..:
            // intentional fall-through:
        case HeaderPath::IncludePath:
            prefix = QLatin1String("-I");
            break;
        }

        result.append(prefix + headerPath.path);
    }

#if 0
    qDebug() << "--- m_args:";
    foreach (const QString &arg, result)
        qDebug() << "\t" << qPrintable(arg);
    qDebug() << "---";
#endif

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

/// @return "-std" flag to select standard, flags for C extensions
/// @return "-std" flag to select standard, flags for C++ extensions, Qt injections
QStringList clangOptionsForLanguage(CppTools::ProjectPart::QtVersion qtVersion,
                                    CppTools::ProjectPart::LanguageVersion languageVersion,
                                    ProjectPart::LanguageExtensions languageExtensions)
{
    QStringList opts;
    bool gnuExtensions = languageExtensions & ProjectPart::GnuExtensions;
    switch (languageVersion) {
    case ProjectPart::C89:
        opts << (gnuExtensions ? QLatin1String("-std=gnu89") : QLatin1String("-std=c89"));
        break;
    case ProjectPart::C99:
        opts << (gnuExtensions ? QLatin1String("-std=gnu99") : QLatin1String("-std=c99"));
        break;
    case ProjectPart::C11:
        opts << (gnuExtensions ? QLatin1String("-std=gnu11") : QLatin1String("-std=c11"));
        break;
    case ProjectPart::CXX11:
        opts << (gnuExtensions ? QLatin1String("-std=gnu++11") : QLatin1String("-std=c++11"));
        break;
    case ProjectPart::CXX98:
        opts << (gnuExtensions ? QLatin1String("-std=gnu++98") : QLatin1String("-std=c++98"));
        break;
    case ProjectPart::CXX03:
        opts << QLatin1String("-std=c++03");
        break;
    case ProjectPart::CXX14:
        opts << QLatin1String("-std=c++1y"); // TODO: change to c++14 after 3.5
        break;
    case ProjectPart::CXX17:
        opts << QLatin1String("-std=c++1z"); // TODO: change to c++17 at some point in the future
        break;
    }

    if (languageExtensions & ProjectPart::MicrosoftExtensions)
        opts << QLatin1String("-fms-extensions");

#if defined(CINDEX_VERSION) // clang 3.2 or higher
    if (languageExtensions & ProjectPart::BorlandExtensions)
        opts << QLatin1String("-fborland-extensions");
#endif

    static const QString injectedHeader(Core::ICore::instance()->resourcePath() + QLatin1String("/cplusplus/qt%1-qobjectdefs-injected.h"));
//    if (qtVersion == ProjectPart::Qt4)
//        opts << QLatin1String("-include") << injectedHeader.arg(QLatin1Char('4'));
    if (qtVersion == ProjectPart::Qt5)
        opts << QLatin1String("-include") << injectedHeader.arg(QLatin1Char('5'));

    return opts;
}

/// @return "-x language-code"
QStringList clangLanguageOption(ProjectFile::Kind fileKind, bool objcExt)
{
    QStringList opts;
    opts += QLatin1String("-x");

    switch (fileKind) {
    case ProjectFile::CHeader:
        if (objcExt)
            opts += QLatin1String("objective-c-header");
        else
            opts += QLatin1String("c-header");
        break;

    case ProjectFile::CXXHeader:
    default:
        if (!objcExt) {
            opts += QLatin1String("c++-header");
            break;
        } // else: fall-through!
    case ProjectFile::ObjCHeader:
    case ProjectFile::ObjCXXHeader:
        opts += QLatin1String("objective-c++-header");
        break;

    case ProjectFile::CSource:
        if (!objcExt) {
            opts += QLatin1String("c");
            break;
        } // else: fall-through!
    case ProjectFile::ObjCSource:
        opts += QLatin1String("objective-c");
        break;

    case ProjectFile::CXXSource:
        if (!objcExt) {
            opts += QLatin1String("c++");
            break;
        } // else: fall-through!
    case ProjectFile::ObjCXXSource:
        opts += QLatin1String("objective-c++");
        break;

    case ProjectFile::OpenCLSource:
        opts += QLatin1String("cl");
        break;
    case ProjectFile::CudaSource:
        opts += QLatin1String("cuda");
        break;
    }

    return opts;
}

} // namespace Utils
} // namespace Clang
