/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef ABSTRACTMAEMODEPLOYSTEP_H
#define ABSTRACTMAEMODEPLOYSTEP_H

#include "abstractlinuxdevicedeploystep.h"
#include "maemodeployable.h"
#include "maemodeployables.h"
#include "maemodeviceconfigurations.h"

#include <projectexplorer/buildstep.h>

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QPair>
#include <QtCore/QSharedPointer>

QT_BEGIN_NAMESPACE
class QEventLoop;
QT_END_NAMESPACE

namespace Utils { class SshConnection; }

namespace Qt4ProjectManager {
class Qt4BuildConfiguration;
namespace Internal {
class AbstractMaemoPackageCreationStep;
class MaemoDeviceConfig;
class Qt4MaemoDeployConfiguration;

class AbstractMaemoDeployStep
    : public ProjectExplorer::BuildStep, public AbstractLinuxDeviceDeployStep
{
    Q_OBJECT
public:
    virtual ~AbstractMaemoDeployStep();

    Q_INVOKABLE void stop();

signals:
    void done();
    void error();

protected:
    AbstractMaemoDeployStep(ProjectExplorer::BuildStepList *bc,
        const QString &id);
    AbstractMaemoDeployStep(ProjectExplorer::BuildStepList *bc,
        AbstractMaemoDeployStep *other);

    enum BaseState { BaseInactive, StopRequested, Connecting, Deploying };
    BaseState baseState() const { return m_baseState; }

    bool currentlyNeedsDeployment(const QString &host,
        const MaemoDeployable &deployable) const;
    void setDeployed(const QString &host, const MaemoDeployable &deployable);
    void raiseError(const QString &error);
    void writeOutput(const QString &text, OutputFormat format = MessageOutput,
        OutputNewlineSetting newlineSetting = DoAppendNewline);
    void setDeploymentFinished();

    virtual const AbstractMaemoPackageCreationStep *packagingStep() const=0;

    QString deployMountPoint() const;
    const Qt4BuildConfiguration *qt4BuildConfiguration() const;
    QSharedPointer<Utils::SshConnection> connection() const { return m_connection; }

private slots:
    void start();
    void handleConnected();
    void handleConnectionFailure();
    void handleProgressReport(const QString &progressMsg);
    void handleRemoteStdout(const QString &output);
    void handleRemoteStderr(const QString &output);

private:
    virtual bool init();
    virtual void run(QFutureInterface<bool> &fi);
    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    virtual QVariantMap toMap() const;
    virtual bool fromMap(const QVariantMap &map);

    virtual bool isDeploymentNeeded(const QString &hostName) const=0;
    virtual void startInternal()=0;
    virtual void stopInternal()=0;

    void baseCtor();
    void addDeployTimesToMap(QVariantMap &map) const;
    void getDeployTimesFromMap(const QVariantMap &map);
    void connectToDevice();
    void setBaseState(BaseState newState);

    QSharedPointer<Utils::SshConnection> m_connection;
    typedef QPair<MaemoDeployable, QString> DeployablePerHost;
    QHash<DeployablePerHost, QDateTime> m_lastDeployed;
    BaseState m_baseState;
    bool m_hasError;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // ABSTRACTMAEMODEPLOYSTEP_H
