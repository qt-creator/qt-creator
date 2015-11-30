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
    m_aspect->m_qmlDebugServerPort = port;
}

void DebuggerRunConfigWidget::useCppDebuggerClicked(bool on)
{
    m_aspect->m_useCppDebugger = on
            ? DebuggerRunConfigurationAspect::EnabledLanguage
            : DebuggerRunConfigurationAspect::DisabledLanguage;
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
    m_aspect->m_useQmlDebugger = on
            ? DebuggerRunConfigurationAspect::EnabledLanguage
            : DebuggerRunConfigurationAspect::DisabledLanguage;
    if (!on && !m_useCppDebugger->isChecked()) {
        m_useCppDebugger->setChecked(true);
        useCppDebuggerClicked(true);
    }
}

void DebuggerRunConfigWidget::useMultiProcessToggled(bool on)
{
    m_aspect->m_useMultiProcess = on;
}

} // namespace Internal

/*!
    \class Debugger::DebuggerRunConfigurationAspect
*/

DebuggerRunConfigurationAspect::DebuggerRunConfigurationAspect(
        RunConfiguration *rc) :
    IRunConfigurationAspect(rc),
    m_useCppDebugger(AutoEnabledLanguage),
    m_useQmlDebugger(AutoEnabledLanguage),
    m_qmlDebugServerPort(Constants::QML_DEFAULT_DEBUG_SERVER_PORT),
    m_useMultiProcess(false)
{
    setId("DebuggerAspect");
    setDisplayName(tr("Debugger settings"));
}

void DebuggerRunConfigurationAspect::setUseQmlDebugger(bool value)
{
    m_useQmlDebugger = value ? EnabledLanguage : DisabledLanguage;
    runConfiguration()->requestRunActionsUpdate();
}

void DebuggerRunConfigurationAspect::setUseCppDebugger(bool value)
{
    m_useCppDebugger = value ? EnabledLanguage : DisabledLanguage;
    runConfiguration()->requestRunActionsUpdate();
}

bool DebuggerRunConfigurationAspect::useCppDebugger() const
{
    if (m_useCppDebugger == DebuggerRunConfigurationAspect::AutoEnabledLanguage)
        return runConfiguration()->target()->project()->projectLanguages().contains(
                    ProjectExplorer::Constants::LANG_CXX);
    return m_useCppDebugger == DebuggerRunConfigurationAspect::EnabledLanguage;
}

bool DebuggerRunConfigurationAspect::useQmlDebugger() const
{
    if (m_useQmlDebugger == DebuggerRunConfigurationAspect::AutoEnabledLanguage) {
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

        const Core::Context languages = runConfiguration()->target()->project()->projectLanguages();
        return languages.contains(ProjectExplorer::Constants::LANG_QMLJS)
            && !languages.contains(ProjectExplorer::Constants::LANG_CXX);
    }
    return m_useQmlDebugger == DebuggerRunConfigurationAspect::EnabledLanguage;
}

uint DebuggerRunConfigurationAspect::qmlDebugServerPort() const
{
    return m_qmlDebugServerPort;
}

void DebuggerRunConfigurationAspect::setQmllDebugServerPort(uint port)
{
    m_qmlDebugServerPort = port;
}

bool DebuggerRunConfigurationAspect::useMultiProcess() const
{
    return m_useMultiProcess;
}

void DebuggerRunConfigurationAspect::setUseMultiProcess(bool value)
{
    m_useMultiProcess = value;
}

bool DebuggerRunConfigurationAspect::isQmlDebuggingSpinboxSuppressed() const
{
    Kit *k = runConfiguration()->target()->kit();
    IDevice::ConstPtr dev = DeviceKitInformation::device(k);
    if (dev.isNull())
        return false;
    return dev->canAutoDetectPorts();
}

void DebuggerRunConfigurationAspect::toMap(QVariantMap &map) const
{
    map.insert(QLatin1String(USE_CPP_DEBUGGER_KEY), m_useCppDebugger == EnabledLanguage);
    map.insert(QLatin1String(USE_CPP_DEBUGGER_AUTO_KEY), m_useCppDebugger == AutoEnabledLanguage);
    map.insert(QLatin1String(USE_QML_DEBUGGER_KEY), m_useQmlDebugger == EnabledLanguage);
    map.insert(QLatin1String(USE_QML_DEBUGGER_AUTO_KEY), m_useQmlDebugger == AutoEnabledLanguage);
    map.insert(QLatin1String(QML_DEBUG_SERVER_PORT_KEY), m_qmlDebugServerPort);
    map.insert(QLatin1String(USE_MULTIPROCESS_KEY), m_useMultiProcess);
}

void DebuggerRunConfigurationAspect::fromMap(const QVariantMap &map)
{
    if (map.value(QLatin1String(USE_CPP_DEBUGGER_AUTO_KEY), false).toBool()) {
        m_useCppDebugger = AutoEnabledLanguage;
    } else {
        bool useCpp = map.value(QLatin1String(USE_CPP_DEBUGGER_KEY), false).toBool();
        m_useCppDebugger = useCpp ? EnabledLanguage : DisabledLanguage;
    }
    if (map.value(QLatin1String(USE_QML_DEBUGGER_AUTO_KEY), false).toBool()) {
        m_useQmlDebugger = AutoEnabledLanguage;
    } else {
        bool useQml = map.value(QLatin1String(USE_QML_DEBUGGER_KEY), false).toBool();
        m_useQmlDebugger = useQml ? EnabledLanguage : DisabledLanguage;
    }
    m_useMultiProcess = map.value(QLatin1String(USE_MULTIPROCESS_KEY), false).toBool();
}

DebuggerRunConfigurationAspect *DebuggerRunConfigurationAspect::create
    (RunConfiguration *runConfiguration) const
{
    return new DebuggerRunConfigurationAspect(runConfiguration);
}

RunConfigWidget *DebuggerRunConfigurationAspect::createConfigurationWidget()
{
    return new Internal::DebuggerRunConfigWidget(this);
}

} // namespace Debugger
