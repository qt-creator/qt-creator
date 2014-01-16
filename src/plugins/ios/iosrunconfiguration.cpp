/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "iosrunconfiguration.h"
#include "iosmanager.h"
#include "iosdeploystep.h"

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/buildsteplist.h>
#include <qmakeprojectmanager/qmakebuildconfiguration.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <qmakeprojectmanager/qmakenodes.h>
#include <qtsupport/qtoutputformatter.h>
#include <qtsupport/qtkitinformation.h>
#include "ui_iosrunconfiguration.h"

#include <QList>

#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace QmakeProjectManager;

namespace Ios {
namespace Internal {

const QLatin1String runConfigurationKey("Ios.run_arguments");

class IosRunConfigurationWidget : public RunConfigWidget
{
    Q_OBJECT

public:
    IosRunConfigurationWidget(IosRunConfiguration *runConfiguration);
    ~IosRunConfigurationWidget();
    QString argListToString(const QStringList &args) const;
    QStringList stringToArgList(const QString &args) const;
    QString displayName() const;

private slots:
    void argumentsLineEditTextEdited();
    void updateValues();

private:
    Ui::IosRunConfiguration *m_ui;
    IosRunConfiguration *m_runConfiguration;
};

IosRunConfiguration::IosRunConfiguration(Target *parent, Core::Id id, const QString &path)
    : RunConfiguration(parent, id)
    , m_profilePath(path)
{
    init();
}

IosRunConfiguration::IosRunConfiguration(Target *parent, IosRunConfiguration *source)
    : RunConfiguration(parent, source)
    , m_profilePath(source->m_profilePath)
    , m_arguments(source->m_arguments)
{
    init();
}

void IosRunConfiguration::init()
{
    setDefaultDisplayName(defaultDisplayName());
}

QWidget *IosRunConfiguration::createConfigurationWidget()
{
    return new IosRunConfigurationWidget(this);
}

Utils::OutputFormatter *IosRunConfiguration::createOutputFormatter() const
{
    return new QtSupport::QtOutputFormatter(target()->project());
}

QStringList IosRunConfiguration::commandLineArguments()
{
    return m_arguments;
}

QString IosRunConfiguration::defaultDisplayName()
{
    ProjectExplorer::IDevice::ConstPtr dev =
            ProjectExplorer::DeviceKitInformation::device(target()->kit());
    const QString devName = dev.isNull() ? IosDevice::name() : dev->displayName();
    return tr("Run on %1").arg(devName);
}

IosDeployStep *IosRunConfiguration::deployStep() const
{
    IosDeployStep * step = 0;
    DeployConfiguration *config = target()->activeDeployConfiguration();
    ProjectExplorer::BuildStepList *bsl = config->stepList();
    if (bsl) {
        const QList<ProjectExplorer::BuildStep *> &buildSteps = bsl->steps();
        for (int i = buildSteps.count() - 1; i >= 0; --i) {
            step = qobject_cast<IosDeployStep *>(buildSteps.at(i));
            if (step)
                break;
        }
    }
    Q_ASSERT_X(step, Q_FUNC_INFO, "Impossible: iOS build configuration without deploy step.");
    return step;
}

QString IosRunConfiguration::profilePath() const
{
    return m_profilePath;
}

QString IosRunConfiguration::appName() const
{
    QmakeProject *pro = qobject_cast<QmakeProject *>(target()->project());
    if (pro) {
        const QmakeProFileNode *node = pro->rootQmakeProjectNode()->findProFileFor(profilePath());
        if (node) {
            TargetInformation ti = node->targetInformation();
            if (ti.valid)
                return ti.target;
        }
    }
    qDebug() << "IosRunConfiguration::appName failed";
    return QString();
}

Utils::FileName IosRunConfiguration::bundleDir() const
{
    Utils::FileName res;
    Core::Id devType = ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(target()->kit());
    bool isDevice = (devType == Constants::IOS_DEVICE_TYPE);
    if (!isDevice && devType != Constants::IOS_SIMULATOR_TYPE) {
        qDebug() << "unexpected device type in bundleDirForTarget: " << devType.toString();
        return res;
    }
    QmakeBuildConfiguration *bc =
            qobject_cast<QmakeBuildConfiguration *>(target()->activeBuildConfiguration());
    if (bc) {
        res = bc->buildDirectory();
        switch (bc->buildType()) {
        case BuildConfiguration::Debug :
        case BuildConfiguration::Unknown :
            if (isDevice)
                res.appendPath(QLatin1String("Debug-iphoneos"));
            else
                res.appendPath(QLatin1String("Debug-iphonesimulator"));
            break;
        case BuildConfiguration::Release :
            if (isDevice)
                res.appendPath(QLatin1String("Release-iphoneos"));
            else
                res.appendPath(QLatin1String("/Release-iphonesimulator"));
            break;
        default:
            qDebug() << "IosBuildStep had an unknown buildType "
                     << target()->activeBuildConfiguration()->buildType();
        }
    }
    res.appendPath(appName() + QLatin1String(".app"));
    return res;
}

Utils::FileName IosRunConfiguration::exePath() const
{
    return bundleDir().appendPath(appName());
}

bool IosRunConfiguration::fromMap(const QVariantMap &map)
{
    m_arguments = map.value(runConfigurationKey).toStringList();
    return RunConfiguration::fromMap(map);
}

QVariantMap IosRunConfiguration::toMap() const
{
    QVariantMap res = RunConfiguration::toMap();
    res[runConfigurationKey] = m_arguments;
    return res;
}

IosRunConfigurationWidget::IosRunConfigurationWidget(IosRunConfiguration *runConfiguration) :
    m_ui(new Ui::IosRunConfiguration), m_runConfiguration(runConfiguration)
{
    m_ui->setupUi(this);

    updateValues();
    connect(m_ui->argumentsLineEdit, SIGNAL(editingFinished()),
            SLOT(argumentsLineEditTextEdited()));
    connect(runConfiguration->target(), SIGNAL(buildDirectoryChanged()),
            SLOT(updateValues()));
}

IosRunConfigurationWidget::~IosRunConfigurationWidget()
{
    delete m_ui;
}

QString IosRunConfigurationWidget::argListToString(const QStringList &args) const
{
    return Utils::QtcProcess::joinArgs(args);
}

QStringList IosRunConfigurationWidget::stringToArgList(const QString &args) const
{
    Utils::QtcProcess::SplitError err;
    QStringList res = Utils::QtcProcess::splitArgs(args, false, &err);
    switch (err) {
    case Utils::QtcProcess::SplitOk:
        break;
    case Utils::QtcProcess::BadQuoting:
        if (args.at(args.size()-1) == QLatin1Char('\\')) {
            res = Utils::QtcProcess::splitArgs(args + QLatin1Char('\\'), false, &err);
            if (err != Utils::QtcProcess::SplitOk)
                res = Utils::QtcProcess::splitArgs(args + QLatin1Char('\\') + QLatin1Char('\''),
                                                   false, &err);
            if (err != Utils::QtcProcess::SplitOk)
                res = Utils::QtcProcess::splitArgs(args + QLatin1Char('\\') + QLatin1Char('\"'),
                                                   false, &err);
        }
        if (err != Utils::QtcProcess::SplitOk)
            res = Utils::QtcProcess::splitArgs(args + QLatin1Char('\''), false, &err);
        if (err != Utils::QtcProcess::SplitOk)
            res = Utils::QtcProcess::splitArgs(args + QLatin1Char('\"'), false, &err);
        break;
    case Utils::QtcProcess::FoundMeta:
        qDebug() << "IosRunConfigurationWidget FoundMeta (should not happen)";
        break;
    }
    return res;
}

QString IosRunConfigurationWidget::displayName() const
{
    return tr("iOS run settings");
}

void IosRunConfigurationWidget::argumentsLineEditTextEdited()
{
    QString argsString = m_ui->argumentsLineEdit->text();
    QStringList args = stringToArgList(argsString);
    m_runConfiguration->m_arguments = args;
    m_ui->argumentsLineEdit->setText(argListToString(args));
}

void IosRunConfigurationWidget::updateValues()
{
    QStringList args = m_runConfiguration->m_arguments;
    QString argsString = argListToString(args);
    m_ui->argumentsLineEdit->setText(argsString);
    m_ui->executableLineEdit->setText(m_runConfiguration->exePath().toUserOutput());
}

} // namespace Internal
} // namespace Ios

#include "iosrunconfiguration.moc"
