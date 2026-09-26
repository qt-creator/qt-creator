// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "java_global.h"

#include <projectexplorer/ioutputparser.h>

#include <utils/filepath.h>

namespace Java {

// The diagnostics of javac, which it prints as
//
//     Greeter.java:11: error: cannot find symbol
//
// and which Ant and Gradle print with a "[javac]" in front of.
class JAVA_EXPORT JavaParser : public ProjectExplorer::OutputTaskParser
{
public:
    JavaParser();

    // A build that compiles copies of the sources rather than the sources
    // themselves - as the Android package build does - reports the copy. These
    // say where to look for the file the user has.
    void setProjectFileList(const Utils::FilePaths &fileList);
    void setBuildDirectory(const Utils::FilePath &buildDirectory);
    void setSourceDirectory(const Utils::FilePath &sourceDirectory);

private:
    Result handleLine(const QString &line, Utils::OutputFormat type) final;

    Utils::FilePaths m_fileList;
    Utils::FilePath m_sourceDirectory;
    Utils::FilePath m_buildDirectory;
};

#ifdef WITH_TESTS
namespace Internal { QObject *createJavaParserTest(); }
#endif

} // namespace Java
