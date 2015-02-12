/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef WINRTPACKAGEDEPLOYMENTSTEP_H
#define WINRTPACKAGEDEPLOYMENTSTEP_H

#include <projectexplorer/abstractprocessstep.h>

namespace WinRt {
namespace Internal {

class WinRtPackageDeploymentStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
public:
    explicit WinRtPackageDeploymentStep(ProjectExplorer::BuildStepList *bsl);
    bool init();
    void run(QFutureInterface<bool> &fi);
    bool processSucceeded(int exitCode, QProcess::ExitStatus status);
    void stdOutput(const QString &line);
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget();

    void setWinDeployQtArguments(const QString &args);
    QString winDeployQtArguments() const;
    QString defaultWinDeployQtArguments() const;

    void raiseError(const QString &errorMessage);
    void raiseWarning(const QString &warningMessage);

    bool fromMap(const QVariantMap &map);
    QVariantMap toMap() const;

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

#endif // WINRTPACKAGEDEPLOYMENTSTEP_H
