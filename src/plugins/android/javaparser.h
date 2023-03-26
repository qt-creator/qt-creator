// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/ioutputparser.h>

#include <utils/filepath.h>

namespace Android::Internal {

class JavaParser : public ProjectExplorer::OutputTaskParser
{
public:
    JavaParser();

    void setProjectFileList(const Utils::FilePaths &fileList);
    void setBuildDirectory(const Utils::FilePath &buildDirectory);
    void setSourceDirectory(const Utils::FilePath &sourceDirectory);

private:
    Result handleLine(const QString &line, Utils::OutputFormat type) final;

    Utils::FilePaths m_fileList;
    Utils::FilePath m_sourceDirectory;
    Utils::FilePath m_buildDirectory;
};

} // Android::Internal
