// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/ioutputparser.h>
#include <utils/fileutils.h>

#include <QRegularExpression>

namespace Android {
namespace Internal {

class JavaParser : public ProjectExplorer::OutputTaskParser
{
    Q_OBJECT

public:
    JavaParser();

    void setProjectFileList(const Utils::FilePaths &fileList);
    void setBuildDirectory(const Utils::FilePath &buildDirectory);
    void setSourceDirectory(const Utils::FilePath &sourceDirectory);

private:
    Result handleLine(const QString &line, Utils::OutputFormat type) override;

    const QRegularExpression m_javaRegExp;
    Utils::FilePaths m_fileList;
    Utils::FilePath m_sourceDirectory;
    Utils::FilePath m_buildDirectory;
};

} // namespace Internal
} // namespace Android
