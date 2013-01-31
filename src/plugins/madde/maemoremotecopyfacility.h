/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef MAEMOREMOTECOPYFACILITY_H
#define MAEMOREMOTECOPYFACILITY_H

#include <projectexplorer/deployablefile.h>
#include <projectexplorer/devicesupport/idevice.h>

#include <QList>
#include <QSharedPointer>

namespace QSsh {
class SshConnection;
class SshRemoteProcessRunner;
}

namespace Madde {
namespace Internal {

class MaemoRemoteCopyFacility : public QObject
{
    Q_OBJECT
public:
    explicit MaemoRemoteCopyFacility(QObject *parent = 0);
    ~MaemoRemoteCopyFacility();

    void copyFiles(QSsh::SshConnection *connection,
        const ProjectExplorer::IDevice::ConstPtr &device,
        const QList<ProjectExplorer::DeployableFile> &deployables, const QString &mountPoint);
    void cancel();

signals:
    void stdoutData(const QString &output);
    void stderrData(const QString &output);
    void progress(const QString &message);
    void fileCopied(const ProjectExplorer::DeployableFile &deployable);
    void finished(const QString &errorMsg = QString());

private slots:
    void handleConnectionError();
    void handleCopyFinished(int exitStatus);
    void handleRemoteStdout();
    void handleRemoteStderr();

private:
    void copyNextFile();
    void setFinished();

    QSsh::SshRemoteProcessRunner *m_copyRunner;
    QSsh::SshRemoteProcessRunner *m_killProcess;
    ProjectExplorer::IDevice::ConstPtr m_devConf;
    QList<ProjectExplorer::DeployableFile> m_deployables;
    QString m_mountPoint;
    bool m_isCopying; // TODO: Redundant due to being in sync with m_copyRunner?
};

} // namespace Internal
} // namespace Madde

#endif // MAEMOREMOTECOPYFACILITY_H
