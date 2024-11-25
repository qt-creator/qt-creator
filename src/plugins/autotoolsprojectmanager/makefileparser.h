// Copyright (C) 2016 Openismus GmbH.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/projectmacro.h>

#include <QFuture>
#include <QStringList>

namespace AutotoolsProjectManager::Internal {

class MakefileParserOutputData final
{
public:
    // File name of the executable.
    QString m_executable;
    // List of sources that are set for the _SOURCES target.
    // Sources in sub directorties contain the sub directory as prefix.
    QStringList m_sources;
    // List of Makefile.am files from the current directory and all sub directories.
    // The values for sub directories contain the sub directory as prefix.
    QStringList m_makefiles;
    // List of include paths. Should be invoked, after the signal finished() has been emitted.
    QStringList m_includePaths;
    // Concatenated normalized defines, just like in code:
    // #define X12_DEPRECATED __attribute__((deprecated))
    // #define X12_HAS_DEPRECATED
    ProjectExplorer::Macros m_macros;
    // List of compiler flags for C.
    QStringList m_cflags;
    // List of compiler flags for C++.
    QStringList m_cxxflags;
};

std::optional<MakefileParserOutputData> parseMakefile(const QString &makefile,
                                                      const QFuture<void> &future);

} // AutotoolsProjectManager::Internal
