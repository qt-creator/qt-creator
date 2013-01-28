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
#ifndef ABSTRACTREMOTELINUXDEPLOYSTEP_H
#define ABSTRACTREMOTELINUXDEPLOYSTEP_H

#include "remotelinux_export.h"

#include <projectexplorer/buildstep.h>

#include <QVariantMap>

namespace RemoteLinux {
class AbstractRemoteLinuxDeployService;
class RemoteLinuxDeployConfiguration;

namespace Internal {
class AbstractRemoteLinuxDeployStepPrivate;
}

class REMOTELINUX_EXPORT AbstractRemoteLinuxDeployStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT

public:
    ~AbstractRemoteLinuxDeployStep();
    bool fromMap(const QVariantMap &map);
    QVariantMap toMap() const;
    bool init();
    void run(QFutureInterface<bool> &fi);
    bool runInGuiThread() const { return true; }
    void cancel();
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    RemoteLinuxDeployConfiguration *deployConfiguration() const;

    virtual AbstractRemoteLinuxDeployService *deployService() const = 0;

protected:
    AbstractRemoteLinuxDeployStep(ProjectExplorer::BuildStepList *bsl, const Core::Id id);
    AbstractRemoteLinuxDeployStep(ProjectExplorer::BuildStepList *bsl,
        AbstractRemoteLinuxDeployStep *other);
    virtual bool initInternal(QString *error = 0) = 0;

private slots:
    void handleProgressMessage(const QString &message);
    void handleErrorMessage(const QString &message);
    void handleWarningMessage(const QString &message);
    void handleFinished();
    void handleStdOutData(const QString &data);
    void handleStdErrData(const QString &data);

private:
    Internal::AbstractRemoteLinuxDeployStepPrivate *d;
};

} // namespace RemoteLinux

#endif // ABSTRACTREMOTELINUXDEPLOYSTEP_H
