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

#include "debuggerrunconfigurationaspect.h"

#include "debuggerconstants.h"

#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/helpmanager.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/buildsteplist.h>

#include <QCheckBox>
#include <QSpinBox>
#include <QDebug>
#include <QFormLayout>
#include <QLabel>

const char USE_CPP_DEBUGGER_KEY[] = "RunConfiguration.UseCppDebugger";
const char USE_CPP_DEBUGGER_AUTO_KEY[] = "RunConfiguration.UseCppDebuggerAuto";
const char USE_QML_DEBUGGER_KEY[] = "RunConfiguration.UseQmlDebugger";
const char USE_QML_DEBUGGER_AUTO_KEY[] = "RunConfiguration.UseQmlDebuggerAuto";
const char QML_DEBUG_SERVER_PORT_KEY[] = "RunConfiguration.QmlDebugServerPort";
const char USE_MULTIPROCESS_KEY[] = "RunConfiguration.UseMultiProcess";

using namespace ProjectExplorer;

namespace Debugger {
namespace Internal {

////////////////////////////////////////////////////////////////////////
//
// DebuggerRunConfigWidget
//
////////////////////////////////////////////////////////////////////////

class DebuggerRunConfigWidget : public RunConfigWidget
{
    Q_DECLARE_TR_FUNCTIONS(Debugger::Internal::RunConfigWidget)

public:
    explicit DebuggerRunConfigWidget(DebuggerRunConfigurationAspect *aspect);
    QString displayName() const { return tr("Debugger Settings"); }

    void showEvent(QShowEvent *event);
    void update();

    void useCppDebuggerClicked(bool on);
    void useQmlDebuggerToggled(bool on);
    void useQmlDebuggerClicked(bool on);
    void qmlDebugServerPortChanged(int port);
    void useMultiProcessToggled(bool on);

public:
    DebuggerRunConfigurationAspect *m_aspect; // not owned

    QCheckBox *m_useCppDebugger;
    QCheckBox *m_useQmlDebugger;
    QSpinBox *m_debugServerPort;
    QLabel *m_debugServerPortLabel;
    QLabel *m_qmlDebuggerInfoLabel;
    QCheckBox *m_useMultiProcess;
};

DebuggerRunConfigWidget::DebuggerRunConfigWidget(DebuggerRunConfigurationAspect *aspect)
{
    m_aspect = aspect;

    m_useCppDebugger = new QCheckBox(tr("Enable C++"), this);
    m_useQmlDebugger = new QCheckBox(tr("Enable QML"), this);

    m_debugServerPort = new QSpinBox(this);
    m_debugServerPort->setMinimum(1);
    m_debugServerPort->setMaximum(65535);

    m_debugServerPortLabel = new QLabel(tr("Debug port:"), this);
    m_debugServerPortLabel->setBuddy(m_debugServerPort);

    m_qmlDebuggerInfoLabel = new QLabel(tr("<a href=\""
        "qthelp://org.qt-project.qtcreator/doc/creator-debugging-qml.html"
        "\">What are the prerequisites?</a>"));

    static const QByteArray env = qgetenv("QTC_DEBUGGER_MULTIPROCESS");
    m_useMultiProcess =
        new QCheckBox(tr("Enable Debugging of Subprocesses"), this);
    m_useMultiProcess->setVisible(env.toInt());

    connect(m_qmlDebuggerInfoLabel, &QLabel::linkActivated,
            [](const QString &link) { Core::HelpManager::handleHelpRequest(link); });
    connect(m_useQmlDebugger, &QAbstractButton::toggled,
            this, &DebuggerRunConfigWidget::useQmlDebuggerToggled);
    connect(m_useQmlDebugger, &QAbstractButton::clicked,
            this, &DebuggerRunConfigWidget::useQmlDebuggerClicked);
    connect(m_useCppDebugger, &QAbstractButton::clicked,
            this, &DebuggerRunConfigWidget::useCppDebuggerClicked);
    connect(m_debugServerPort, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &DebuggerRunConfigWidget::qmlDebugServerPortChanged);
    connect(m_useMultiProcess, &QAbstractButton::toggled,
            this, &DebuggerRunConfigWidget::useMultiProcessToggled);

    auto qmlLayout = new QHBoxLayout;
    qmlLayout->setMargin(0);
    qmlLayout->addWidget(m_useQmlDebugger);
    qmlLayout->addWidget(m_debugServerPortLabel);
    qmlLayout->addWidget(m_debugServerPort);
    qmlLayout->addWidget(m_qmlDebuggerInfoLabel);
    qmlLayout->addStretch();

    auto layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->addWidget(m_useCppDebugger);
    layout->addLayout(qmlLayout);
    layout->addWidget(m_useMultiProcess);
    setLayout(layout);
}

void DebuggerRunConfigWidget::showEvent(QShowEvent *event)
{
    // Update the UI on every show() because the state of
    // QML debugger language is hard to track.
    //
    // !event->spontaneous makes sure we ignore e.g. global windows events,
    // when Qt Creator itself is minimized/maximized.
    if (!event->spontaneous())
        update();

    RunConfigWidget::showEvent(event);
}

void DebuggerRunConfigWidget::update()
{
    m_useCppDebugger->setChecked(m_aspect->useCppDebugger());
    m_useQmlDebugger->setChecked(m_aspect->useQmlDebugger());

    m_debugServerPort->setValue(m_aspect->qmlDebugServerPort());

    m_useMultiProcess->setChecked(m_aspect->useMultiProcess());

    m_debugServerPortLabel->setVisible(!m_aspect->isQmlDebuggingSpinboxSuppressed());
    m_debugServerPort->setVisible(!m_aspect->isQmlDebuggingSpinboxSuppressed());
}

void DebuggerRunConfigWidget::qmlDebugServerPortChanged(int port)
{
    m_aspect->d.qmlDebugServerPort = port;
}

void DebuggerRunConfigWidget::useCppDebuggerClicked(bool on)
{
    m_aspect->d.useCppDebugger = on ? EnabledLanguage : DisabledLanguage;
    if (!on && !m_useQmlDebugger->isChecked()) {
        m_useQmlDebugger->setChecked(true);
        useQmlDebuggerClicked(true);
    }
}

void DebuggerRunConfigWidget::useQmlDebuggerToggled(bool on)
{
    m_debugServerPort->setEnabled(on);
    m_debugServerPortLabel->setEnabled(on);
}

void DebuggerRunConfigWidget::useQmlDebuggerClicked(bool on)
{
    m_aspect->d.useQmlDebugger = on ? EnabledLanguage : DisabledLanguage;
    if (!on && !m_useCppDebugger->isChecked()) {
        m_useCppDebugger->setChecked(true);
        useCppDebuggerClicked(true);
    }
}

void DebuggerRunConfigWidget::useMultiProcessToggled(bool on)
{
    m_aspect->d.useMultiProcess = on;
}

} // namespace Internal

/*!
    \class Debugger::DebuggerRunConfigurationAspect
*/

DebuggerRunConfigurationAspect::DebuggerRunConfigurationAspect(
        RunConfiguration *rc) :
    IRunConfigurationAspect(rc)
{
    setId("DebuggerAspect");
    setDisplayName(tr("Debugger settings"));
    setRunConfigWidgetCreator([this] { return new Internal::DebuggerRunConfigWidget(this); });
}

void DebuggerRunConfigurationAspect::setUseQmlDebugger(bool value)
{
    d.useQmlDebugger = value ? EnabledLanguage : DisabledLanguage;
    runConfiguration()->requestRunActionsUpdate();
}

void DebuggerRunConfigurationAspect::setUseCppDebugger(bool value)
{
    d.useCppDebugger = value ? EnabledLanguage : DisabledLanguage;
    runConfiguration()->requestRunActionsUpdate();
}

bool DebuggerRunConfigurationAspect::useCppDebugger() const
{
    if (d.useCppDebugger == AutoEnabledLanguage)
        return runConfiguration()->target()->project()->projectLanguages().contains(
                    ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    return d.useCppDebugger == EnabledLanguage;
}

bool DebuggerRunConfigurationAspect::useQmlDebugger() const
{
    if (d.useQmlDebugger == AutoEnabledLanguage) {
        const Core::Context languages = runConfiguration()->target()->project()->projectLanguages();
        if (!languages.contains(ProjectExplorer::Constants::QMLJS_LANGUAGE_ID))
            return false;

        //
        // Try to find a build step (qmake) to check whether qml debugging is enabled there
        // (Using the Qt metatype system to avoid a hard qt4projectmanager dependency)
        //
        if (BuildConfiguration *bc = runConfiguration()->target()->activeBuildConfiguration()) {
            if (BuildStepList *bsl = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD)) {
                foreach (BuildStep *step, bsl->steps()) {
                    QVariant linkProperty = step->property("linkQmlDebuggingLibrary");
                    if (linkProperty.isValid() && linkProperty.canConvert(QVariant::Bool))
                        return linkProperty.toBool();
                }
            }
        }

        return !languages.contains(ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    }
    return d.useQmlDebugger == EnabledLanguage;
}

uint DebuggerRunConfigurationAspect::qmlDebugServerPort() const
{
    return d.qmlDebugServerPort;
}

void DebuggerRunConfigurationAspect::setQmllDebugServerPort(uint port)
{
    d.qmlDebugServerPort = port;
}

bool DebuggerRunConfigurationAspect::useMultiProcess() const
{
    return d.useMultiProcess;
}

void DebuggerRunConfigurationAspect::setUseMultiProcess(bool value)
{
    d.useMultiProcess = value;
}

bool DebuggerRunConfigurationAspect::isQmlDebuggingSpinboxSuppressed() const
{
    Kit *k = runConfiguration()->target()->kit();
    IDevice::ConstPtr dev = DeviceKitInformation::device(k);
    if (dev.isNull())
        return false;
    return dev->canAutoDetectPorts();
}

int DebuggerRunConfigurationAspect::portsUsedByDebugger() const
{
    int ports = 0;
    if (useQmlDebugger())
        ++ports;
    if (useCppDebugger())
        ++ports;
    return ports;
}

void DebuggerRunConfigurationAspect::toMap(QVariantMap &map) const
{
    map.insert(QLatin1String(USE_CPP_DEBUGGER_KEY), d.useCppDebugger == EnabledLanguage);
    map.insert(QLatin1String(USE_CPP_DEBUGGER_AUTO_KEY), d.useCppDebugger == AutoEnabledLanguage);
    map.insert(QLatin1String(USE_QML_DEBUGGER_KEY), d.useQmlDebugger == EnabledLanguage);
    map.insert(QLatin1String(USE_QML_DEBUGGER_AUTO_KEY), d.useQmlDebugger == AutoEnabledLanguage);
    map.insert(QLatin1String(QML_DEBUG_SERVER_PORT_KEY), d.qmlDebugServerPort);
    map.insert(QLatin1String(USE_MULTIPROCESS_KEY), d.useMultiProcess);
}

void DebuggerRunConfigurationAspect::fromMap(const QVariantMap &map)
{
    if (map.value(QLatin1String(USE_CPP_DEBUGGER_AUTO_KEY), false).toBool()) {
        d.useCppDebugger = AutoEnabledLanguage;
    } else {
        bool useCpp = map.value(QLatin1String(USE_CPP_DEBUGGER_KEY), false).toBool();
        d.useCppDebugger = useCpp ? EnabledLanguage : DisabledLanguage;
    }
    if (map.value(QLatin1String(USE_QML_DEBUGGER_AUTO_KEY), false).toBool()) {
        d.useQmlDebugger = AutoEnabledLanguage;
    } else {
        bool useQml = map.value(QLatin1String(USE_QML_DEBUGGER_KEY), false).toBool();
        d.useQmlDebugger = useQml ? EnabledLanguage : DisabledLanguage;
    }
    d.useMultiProcess = map.value(QLatin1String(USE_MULTIPROCESS_KEY), false).toBool();
}

DebuggerRunConfigurationAspect *DebuggerRunConfigurationAspect::create
    (RunConfiguration *runConfiguration) const
{
    return new DebuggerRunConfigurationAspect(runConfiguration);
}

} // namespace Debugger
