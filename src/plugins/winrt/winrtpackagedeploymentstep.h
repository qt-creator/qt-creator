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

#include <projectexplorer/abstractprocessstep.h>

namespace WinRt {
namespace Internal {

class WinRtPackageDeploymentStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
public:
    explicit WinRtPackageDeploymentStep(ProjectExplorer::BuildStepList *bsl);
    bool init(QList<const BuildStep *> &earlierSteps) override;
    void run(QFutureInterface<bool> &fi) override;
    bool processSucceeded(int exitCode, QProcess::ExitStatus status) override;
    void stdOutput(const QString &line) override;
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;

    void setWinDeployQtArguments(const QString &args);
    QString winDeployQtArguments() const;
    QString defaultWinDeployQtArguments() const;

    void raiseError(const QString &errorMessage);
    void raiseWarning(const QString &warningMessage);

    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

private:
    bool parseIconsAndExecutableFromManifest(QString manifestFileName, QStringList *items, QString *executable);

    QString m_args;
    QString m_targetFilePath;
    QString m_targetDirPath;
    QString m_executablePathInManifest;
    QString m_mappingFileContent;
    QString m_manifestFileName;
    bool m_createMappingFile;
};

} // namespace Internal
} // namespace WinRt
