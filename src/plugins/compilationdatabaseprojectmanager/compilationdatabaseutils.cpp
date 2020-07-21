/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "compilationdatabaseutils.h"

#include <projectexplorer/headerpath.h>
#include <projectexplorer/projectmacro.h>

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/optional.h>
#include <utils/stringutils.h>

#include <QDir>
#include <QRegularExpression>
#include <QSet>

using namespace ProjectExplorer;

namespace CompilationDatabaseProjectManager {
namespace Internal {

static QString updatedPathFlag(const QString &pathStr, const QString &workingDir)
{
    QString result = pathStr;
    if (QDir(pathStr).isRelative())
        result = workingDir + "/" + pathStr;

    return result;
}

static CppTools::ProjectFile::Kind fileKindFromString(QString flag)
{
    using namespace CppTools;
    if (flag.startsWith("-x"))
        flag = flag.mid(2);

    if (flag == "c++-header")
        return ProjectFile::CXXHeader;
    if (flag == "c-header")
        return ProjectFile::CHeader;
    if (flag == "c++" || flag == "/TP" || flag.startsWith("/Tp"))
        return ProjectFile::CXXSource;
    if (flag == "c" || flag == "/TC" || flag.startsWith("/Tc"))
        return ProjectFile::CSource;

    if (flag == "objective-c++")
        return ProjectFile::ObjCXXSource;
    if (flag == "objective-c++-header")
        return ProjectFile::ObjCXXHeader;
    if (flag == "objective-c")
        return ProjectFile::ObjCSource;
    if (flag == "objective-c-header")
        return ProjectFile::ObjCHeader;

    if (flag == "cl")
        return ProjectFile::OpenCLSource;
    if (flag == "cuda")
        return ProjectFile::CudaSource;

    return ProjectFile::Unclassified;
}

QStringList filterFromFileName(const QStringList &flags, QString baseName)
{
    baseName.append('.'); // to match name.c, name.o, etc.
    QStringList result;
    result.reserve(flags.size());
    for (const QString &flag : flags) {
        if (!flag.contains(baseName))
            result.push_back(flag);
    }

    return result;
}

void filteredFlags(const QString &fileName,
                   const QString &workingDir,
                   QStringList &flags,
                   HeaderPaths &headerPaths,
                   Macros &macros,
                   CppTools::ProjectFile::Kind &fileKind,
                   QString &sysRoot)
{
    if (flags.empty())
        return;

    // Skip compiler call if present.
    bool skipNext = Utils::HostOsInfo::isWindowsHost()
                ? (!flags.front().startsWith('/') && !flags.front().startsWith('-'))
                : (!flags.front().startsWith('-'));
    Utils::optional<HeaderPathType> includePathType;
    Utils::optional<MacroType> macroType;
    bool fileKindIsNext = false;

    QStringList filtered;
    for (const QString &flag : flags) {
        if (skipNext) {
            skipNext = false;
            continue;
        }

        if (includePathType) {
            const QString pathStr = updatedPathFlag(flag, workingDir);
            headerPaths.append({pathStr, includePathType.value()});
            includePathType.reset();
            continue;
        }

        if (macroType) {
            Macro macro = Macro::fromKeyValue(flag);
            macro.type = macroType.value();
            macros.append(macro);
            macroType.reset();
            continue;
        }

        if (flag != "-x"
                && (fileKindIsNext || flag == "/TC" || flag == "/TP"
                    || flag.startsWith("/Tc") || flag.startsWith("/Tp") || flag.startsWith("-x"))) {
            fileKindIsNext = false;
            fileKind = fileKindFromString(flag);
            continue;
        }

        if (flag == "-x") {
            fileKindIsNext = true;
            continue;
        }

        if (flag == "-o" || flag == "-MF" || flag == "-c" || flag == "-pedantic"
                || flag.startsWith("-O") || flag.startsWith("-W") || flag.startsWith("-w")) {
            continue;
        }

        const QStringList userIncludeFlags{"-I", "/I"};
        const QStringList systemIncludeFlags{"-isystem", "-imsvc", "/imsvc"};
        const QStringList allIncludeFlags = QStringList(userIncludeFlags) << systemIncludeFlags;
        const QString includeOpt = Utils::findOrDefault(allIncludeFlags, [flag](const QString &opt) {
            return flag.startsWith(opt) && flag != opt;
        });
        if (!includeOpt.isEmpty()) {
            const QString pathStr = updatedPathFlag(flag.mid(includeOpt.length()), workingDir);
            headerPaths.append({pathStr, userIncludeFlags.contains(includeOpt)
                                ? HeaderPathType::User : HeaderPathType::System});
            continue;
        }

        if ((flag.startsWith("-D") || flag.startsWith("-U") || flag.startsWith("/D") || flag.startsWith("/U"))
                   && flag != "-D" && flag != "-U" && flag != "/D" && flag != "/U") {
            Macro macro = Macro::fromKeyValue(flag.mid(2));
            macro.type = (flag.startsWith("-D") || flag.startsWith("/D")) ? MacroType::Define : MacroType::Undefine;
            macros.append(macro);
            continue;
        }

        if (userIncludeFlags.contains(flag)) {
            includePathType = HeaderPathType::User;
            continue;
        }
        if (systemIncludeFlags.contains(flag)) {
            includePathType = HeaderPathType::System;
            continue;
        }

        if (flag == "-D" || flag == "-U" || flag == "/D" || flag == "/U") {
            macroType = (flag == "-D" || flag == "/D") ? MacroType::Define : MacroType::Undefine;
            continue;
        }

        if (flag.startsWith("--sysroot=")) {
            if (sysRoot.isEmpty())
                sysRoot = updatedPathFlag(flag.mid(10), workingDir);
            continue;
        }

        if ((flag.startsWith("-std=") || flag.startsWith("/std:"))
                && fileKind == CppTools::ProjectFile::Unclassified) {
            const bool cpp = (flag.contains("c++") || flag.contains("gnu++"));
            if (CppTools::ProjectFile::isHeader(CppTools::ProjectFile::classify(fileName)))
                fileKind = cpp ? CppTools::ProjectFile::CXXHeader : CppTools::ProjectFile::CHeader;
            else
                fileKind = cpp ? CppTools::ProjectFile::CXXSource : CppTools::ProjectFile::CSource;
        }

        // Skip all remaining Windows flags except feature flags.
        if (flag.startsWith("/") && !flag.startsWith("/Z"))
            continue;

        filtered.push_back(flag);
    }

    if (fileKind == CppTools::ProjectFile::Unclassified)
        fileKind = CppTools::ProjectFile::classify(fileName);

    flags = filtered;
}

QStringList splitCommandLine(QString commandLine, QSet<QString> &flagsCache)
{
    QStringList result;
    bool insideQuotes = false;

    // Remove escaped quotes.
    commandLine.replace("\\\"", "'");
    for (const QString &part : commandLine.split(QRegularExpression("\""))) {
        if (insideQuotes) {
            const QString quotedPart = "\"" + part + "\"";
            if (result.last().endsWith("=")) {
                auto flagIt = flagsCache.insert(result.last() + quotedPart);
                result.last() = *flagIt;
            } else {
                auto flagIt = flagsCache.insert(quotedPart);
                result.append(*flagIt);
            }
        } else { // If 's' is outside quotes ...
            for (const QString &flag :
                 part.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts)) {
                auto flagIt = flagsCache.insert(flag);
                result.append(*flagIt);
            }
        }
        insideQuotes = !insideQuotes;
    }
    return result;
}

} // namespace Internal
} // namespace CompilationDatabaseProjectManager
