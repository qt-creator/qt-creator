// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/extracompiler.h>
#include <utils/fileutils.h>
#include <utils/temporarydirectory.h>

namespace QtSupport {

class QScxmlcGenerator : public ProjectExplorer::ProcessExtraCompiler
{
    Q_OBJECT
public:
    QScxmlcGenerator(const ProjectExplorer::Project *project, const Utils::FilePath &source,
                     const Utils::FilePaths &targets, QObject *parent = nullptr);

protected:
    Utils::FilePath command() const override;
    QStringList arguments() const override;
    Utils::FilePath workingDirectory() const override;

private:
    Utils::FilePath tmpFile() const;
    ProjectExplorer::FileNameToContentsHash handleProcessFinished(Utils::QtcProcess *process) override;
    bool prepareToRun(const QByteArray &sourceContents) override;
    ProjectExplorer::Tasks parseIssues(const QByteArray &processStderr) override;

    Utils::TemporaryDirectory m_tmpdir;
    QString m_header;
    QString m_impl;
};

class QScxmlcGeneratorFactory : public ProjectExplorer::ExtraCompilerFactory
{
    Q_OBJECT
public:
    QScxmlcGeneratorFactory() = default;

    ProjectExplorer::FileType sourceType() const override;

    QString sourceTag() const override;

    ProjectExplorer::ExtraCompiler *create(const ProjectExplorer::Project *project,
                                           const Utils::FilePath &source,
                                           const Utils::FilePaths &targets) override;
};

} // QtSupport
