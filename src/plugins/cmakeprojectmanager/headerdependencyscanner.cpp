// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "headerdependencyscanner.h"

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/algorithm.h>
#include <utils/qtcprocess.h>

#include <QCryptographicHash>

using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace Utils;

namespace CMakeProjectManager::Internal {

DependencyDialect dependencyDialect(Id toolchainType)
{
    if (toolchainType == Constants::MSVC_TOOLCHAIN_TYPEID
        || toolchainType == Constants::CLANG_CL_TOOLCHAIN_TYPEID) {
        return DependencyDialect::Msvc;
    }
    return DependencyDialect::Gcc;
}

/*!
    Returns a hash of \a compiler and \a arguments that is stable across runs of
    \QC.

    \c qHash() is seeded per process and would invalidate every stored scan on
    every restart.
*/
quint64 commandHash(const FilePath &compiler, const QStringList &arguments)
{
    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(compiler.toUrlishString().toUtf8());
    for (const QString &argument : arguments) {
        hash.addData(QByteArrayView("\0", 1));
        hash.addData(argument.toUtf8());
    }

    const QByteArray digest = hash.result();
    quint64 result = 0;
    for (int i = 0; i < 8; ++i)
        result = (result << 8) | uchar(digest.at(i));
    return result;
}

SourceScan HeaderScanUnit::toScan() const
{
    return {source, commandHash(compiler, arguments)};
}

/*!
    Removes the arguments from \a arguments that a dependency-only run must not
    see.

    Precompiled headers are dropped because the build system's own \c .gch or
    \c .pch does not belong to this invocation, and the compiler refuses to use
    a stale one. Output and dependency arguments are dropped because they
    collide with the ones added by dependencyScanCommand().
*/
QStringList launderScanArguments(const QStringList &arguments, DependencyDialect dialect)
{
    static const QStringList gccArgumentsWithValue
        = {"-o", "-MF", "-MT", "-MQ", "-include-pch", "--output"};
    static const QStringList gccStandaloneArguments
        = {"-c", "-S", "-M", "-MM", "-MD", "-MMD", "-MG", "-MP", "--preprocess"};
    static const QStringList msvcPrefixesToDrop = {"/Fo", "/Fp", "/Yu", "/Yc", "/showIncludes"};

    QStringList result;
    result.reserve(arguments.size());

    for (int i = 0; i < arguments.size(); ++i) {
        const QString argument = arguments.at(i);

        if (dialect == DependencyDialect::Msvc) {
            QString normalized = argument;
            if (normalized.startsWith('-'))
                normalized[0] = '/';
            if (normalized == "/c" || normalized == "/P" || normalized == "/EP"
                || normalized == "/Zs") {
                continue;
            }
            if (Utils::anyOf(msvcPrefixesToDrop, [&normalized](const QString &prefix) {
                    return normalized.startsWith(prefix);
                })) {
                continue;
            }
            result.append(argument);
            continue;
        }

        if (gccStandaloneArguments.contains(argument))
            continue;

        if (gccArgumentsWithValue.contains(argument)) {
            ++i;
            continue;
        }

        if (argument == "-Xclang" && i + 1 < arguments.size()
            && arguments.at(i + 1) == "-include-pch") {
            i += 3;
            continue;
        }

        if (argument.startsWith("-MF") || argument.startsWith("-MT") || argument.startsWith("-MQ")) {
            continue;
        }

        result.append(argument);
    }

    return result;
}

/*!
    Returns the command line that makes the compiler of \a unit report the
    headers of its source instead of compiling it.

    The GCC dialect writes a make rule to the standard output. \c -MG keeps a
    header that has not been generated yet from turning into an error, so a
    source that includes \c ui_mainwindow.h can be scanned before uic has run.

    The MSVC dialect reports every header on the standard output as it is
    opened. \c /Zs stops after the syntax check, so no object file is produced.
*/
CommandLine dependencyScanCommand(const HeaderScanUnit &unit, DependencyDialect dialect)
{
    CommandLine command(unit.compiler);
    command.addArgs(launderScanArguments(unit.arguments, dialect));

    if (dialect == DependencyDialect::Msvc)
        command.addArgs({"/nologo", "/Zs", "/showIncludes"});
    else
        command.addArgs({"-M", "-MG"});

    command.addArg(unit.source.nativePath());
    return command;
}

/*!
    Returns the dependencies of the make rule in \a output, resolved against
    \a workingDirectory.

    Follows what GCC and Clang produce rather than everything make accepts: a
    backslash escapes a space or a hash sign, an odd number of backslashes
    before a space escapes that space, a trailing backslash continues the line,
    and \c $$ is a literal dollar sign.
*/
FilePaths parseMakeDependencies(const QString &output, const FilePath &workingDirectory)
{
    FilePaths result;
    QSet<QString> seen;
    QString token;
    bool parsingTargets = true;

    const int size = output.size();

    const auto isSeparator = [](QChar c) {
        return c.isNull() || c == ' ' || c == '\t' || c == '\r' || c == '\n';
    };

    const auto flush = [&] {
        if (token.isEmpty())
            return;

        if (token.endsWith(':')) {
            token.chop(1);
            parsingTargets = false;
        } else if (!parsingTargets && !seen.contains(token)) {
            seen.insert(token);
            result.append(workingDirectory.resolvePath(token));
        }
        token.clear();
    };

    int i = 0;
    while (i < size) {
        const QChar c = output.at(i);

        if (c == '\\') {
            int run = 0;
            while (i + run < size && output.at(i + run) == '\\')
                ++run;
            const QChar next = i + run < size ? output.at(i + run) : QChar();

            if (next == ' ') {
                if (run % 2 == 1) {
                    token.append(QString(run / 2, '\\')).append(' ');
                    i += run + 1;
                } else {
                    token.append(QString(run, '\\'));
                    i += run + 1;
                    flush();
                }
                continue;
            }

            if (next == '#') {
                token.append(QString(run - 1, '\\')).append('#');
                i += run + 1;
                continue;
            }

            if (next == ':') {
                const QChar after = i + run + 1 < size ? output.at(i + run + 1) : QChar();
                if (isSeparator(after)) {
                    token.append(QString(run, '\\')).append(':');
                    i += run + 1;
                    flush();
                } else {
                    token.append(QString(run - 1, '\\')).append(':');
                    i += run + 1;
                }
                continue;
            }

            if (run == 1 && (next == '\n' || next == '\r')) {
                flush();
                i += 2;
                if (next == '\r' && i < size && output.at(i) == '\n')
                    ++i;
                continue;
            }

            token.append(QString(run, '\\'));
            i += run;
            continue;
        }

        if (c == '$' && i + 1 < size && output.at(i + 1) == '$') {
            token.append('$');
            i += 2;
            continue;
        }

        if (isSeparator(c)) {
            flush();
            if (c == '\n')
                parsingTargets = true;
            ++i;
            continue;
        }

        token.append(c);
        ++i;
    }

    flush();
    return result;
}

/*!
    Returns \a settings with a \c /showIncludes prefix that the scan can find
    in the output of \c cl.

    CMake detects the prefix, which is localized, but only for the generators
    that read \c /showIncludes themselves. With a Visual Studio generator
    MSBuild does that job, and \c CMAKE_<LANG>_CL_SHOWINCLUDES_PREFIX stays
    empty. Ask \c cl for English messages then, and match the prefix it prints
    in that language.
*/
HeaderScanSettings withShowIncludesPrefix(const HeaderScanSettings &settings)
{
    if (settings.dialect != DependencyDialect::Msvc || !settings.showIncludesPrefix.isEmpty())
        return settings;

    HeaderScanSettings result = settings;
    result.showIncludesPrefix = "Note: including file:";
    result.environment.set("VSLANG", "1033");
    return result;
}

/*!
    Returns the headers that \c cl reported on \a output with \a prefix,
    resolved against \a workingDirectory.

    The prefix is localized, which is why CMake detects it and stores it as
    \c CMAKE_<LANG>_CL_SHOWINCLUDES_PREFIX. Passing that value avoids guessing
    it back from the output.

    Without a prefix there is nothing to recognize. Every line starts with the
    empty string, so matching on it would turn the name of the source and every
    diagnostic into a header.
*/
FilePaths parseShowIncludes(const QString &output,
                            const QString &prefix,
                            const FilePath &workingDirectory)
{
    if (prefix.isEmpty())
        return {};

    FilePaths result;
    QSet<QString> seen;

    const QStringList lines = output.split('\n');
    for (const QString &line : lines) {
        if (!line.startsWith(prefix))
            continue;

        const QString path = line.mid(prefix.size()).trimmed();
        if (path.isEmpty() || seen.contains(path))
            continue;

        seen.insert(path);
        result.append(workingDirectory.resolvePath(path));
    }

    return result;
}

/*!
    Returns a recipe that scans every unit of \a units with \a scanSettings and
    reports each result to \a handler.

    A unit that fails to preprocess is skipped rather than aborting the run:
    its scan simply stays stale and is retried next time. That keeps one source
    with a missing generated header from costing the whole project its scan.

    GCC and Clang write UTF-8. \c cl writes in a code page of the console, which
    the encoding of the locale comes closer to than UTF-8 does.
*/
ExecutableItem headerDependencyScanRecipe(const HeaderScanUnits &units,
                                          const HeaderScanSettings &scanSettings,
                                          const ScanResultHandler &handler)
{
    const HeaderScanSettings settings = withShowIncludesPrefix(scanSettings);
    const ListIterator<HeaderScanUnit> iterator(units);

    const auto onSetup = [iterator, settings](Process &process) {
        process.setCommand(dependencyScanCommand(*iterator, settings.dialect));
        process.setWorkingDirectory(settings.workingDirectory);
        process.setEnvironment(settings.environment);
        if (settings.dialect != DependencyDialect::Msvc)
            process.setUtf8Codec();
    };

    const auto onDone = [iterator, settings, handler](const Process &process) {
        const FilePaths dependencies
            = settings.dialect == DependencyDialect::Msvc
                  ? parseShowIncludes(process.cleanedStdOut(),
                                      settings.showIncludesPrefix,
                                      settings.workingDirectory)
                  : parseMakeDependencies(process.cleanedStdOut(), settings.workingDirectory);

        if (!dependencies.isEmpty())
            handler(iterator->toScan(), dependencies);
    };

    return For (iterator) >> Do {
        finishAllAndSuccess,
        parallelIdealThreadCountLimit,
        ProcessTask(onSetup, onDone, CallDoneFlag::OnSuccess)
    };
}

} // namespace CMakeProjectManager::Internal
