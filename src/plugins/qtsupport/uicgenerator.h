/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#pragma once

#include <projectexplorer/extracompiler.h>
#include <utils/fileutils.h>

#include <QProcess>

namespace QtSupport {

class UicGenerator : public ProjectExplorer::ProcessExtraCompiler
{
    Q_OBJECT
public:
    UicGenerator(const ProjectExplorer::Project *project, const Utils::FilePath &source,
                 const Utils::FilePaths &targets, QObject *parent = nullptr);

protected:
    Utils::FilePath command() const override;
    QStringList arguments() const override;
    ProjectExplorer::FileNameToContentsHash handleProcessFinished(QProcess *process) override;
    void handleProcessStarted(QProcess *process, const QByteArray &sourceContents) override;
};

class UicGeneratorFactory : public ProjectExplorer::ExtraCompilerFactory
{
    Q_OBJECT
public:
    UicGeneratorFactory() = default;

    ProjectExplorer::FileType sourceType() const override;

    QString sourceTag() const override;

    ProjectExplorer::ExtraCompiler *create(const ProjectExplorer::Project *project,
                                           const Utils::FilePath &source,
                                           const Utils::FilePaths &targets) override;
};

} // QtSupport
