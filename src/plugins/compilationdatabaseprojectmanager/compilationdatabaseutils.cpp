// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "compilationdatabaseutils.h"

#include <projectexplorer/headerpath.h>
#include <projectexplorer/projectmacro.h>

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/stringutils.h>

#include <QDir>
#include <QRegularExpression>
#include <QSet>

#include <optional>

using namespace ProjectExplorer;
using namespace Utils;

namespace CompilationDatabaseProjectManager {
namespace Internal {

static CppEditor::ProjectFile::Kind fileKindFromString(QString flag)
{
    using namespace CppEditor;
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

QStringList filterFromFileName(const QStringList &flags, const QString &fileName)
{
    QStringList result;
    result.reserve(flags.size());
    bool skipNext = false;
    for (int i = 0; i < flags.size(); ++i) {
        const QString &flag = flags.at(i);
        if (skipNext) {
            skipNext = false;
            continue;
        }
        if (FilePath::fromUserInput(flag).fileName() == fileName)
            continue;
        if (flag == "-o" || flag.startsWith("/Fo")) {
            skipNext = true;
            continue;
        }
        result.push_back(flag);
    }

    return result;
}

void filteredFlags(const FilePath &filePath,
                   const FilePath &workingDir,
                   QStringList &flags,
                   HeaderPaths &headerPaths,
                   Macros &macros,
                   CppEditor::ProjectFile::Kind &fileKind,
                   FilePath &sysRoot)
{
    if (flags.empty())
        return;

    // Skip compiler call if present.
    bool skipNext = Utils::HostOsInfo::isWindowsHost()
                ? (!flags.front().startsWith('/') && !flags.front().startsWith('-'))
                : (!flags.front().startsWith('-'));
    std::optional<HeaderPathType> includePathType;
    std::optional<MacroType> macroType;
    bool fileKindIsNext = false;

    QStringList filtered;
    for (const QString &flag : flags) {
        if (skipNext) {
            skipNext = false;
            continue;
        }

        if (includePathType) {
            const QString pathStr = workingDir.resolvePath(flag).toString();
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

        const QStringList userIncludeFlags{"-I", "-iquote", "/I"};
        const QStringList systemIncludeFlags{"-isystem", "-idirafter", "-imsvc", "/imsvc"};
        const QStringList allIncludeFlags = QStringList(userIncludeFlags) << systemIncludeFlags;
        const QString includeOpt = Utils::findOrDefault(allIncludeFlags, [flag](const QString &opt) {
            return flag.startsWith(opt) && flag != opt;
        });
        if (!includeOpt.isEmpty()) {
            const QString pathStr = workingDir.resolvePath(flag.mid(includeOpt.length())).toString();
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
                sysRoot = workingDir.resolvePath(flag.mid(10));
            continue;
        }

        if ((flag.startsWith("-std=") || flag.startsWith("/std:"))
                && fileKind == CppEditor::ProjectFile::Unclassified) {
            const bool cpp = (flag.contains("c++") || flag.contains("gnu++"));
            if (CppEditor::ProjectFile::isHeader(CppEditor::ProjectFile::classify(filePath.path())))
                fileKind = cpp ? CppEditor::ProjectFile::CXXHeader : CppEditor::ProjectFile::CHeader;
            else
                fileKind = cpp ? CppEditor::ProjectFile::CXXSource : CppEditor::ProjectFile::CSource;
        }

        // Skip all remaining Windows flags except feature flags.
        if (Utils::HostOsInfo::isWindowsHost() && flag.startsWith("/") && !flag.startsWith("/Z"))
            continue;

        filtered.push_back(flag);
    }

    if (fileKind == CppEditor::ProjectFile::Unclassified)
        fileKind = CppEditor::ProjectFile::classify(filePath.path());

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
