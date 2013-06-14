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

#include <QCheckBox>
#include <QSpinBox>
#include <QDebug>
#include <QFormLayout>
#include <QLabel>

static const char USE_CPP_DEBUGGER_KEY[] = "RunConfiguration.UseCppDebugger";
static const char USE_CPP_DEBUGGER_AUTO_KEY[] = "RunConfiguration.UseCppDebuggerAuto";
static const char USE_QML_DEBUGGER_KEY[] = "RunConfiguration.UseQmlDebugger";
static const char USE_QML_DEBUGGER_AUTO_KEY[] = "RunConfiguration.UseQmlDebuggerAuto";
static const char QML_DEBUG_SERVER_PORT_KEY[] = "RunConfiguration.QmlDebugServerPort";
static const char USE_MULTIPROCESS_KEY[] = "RunConfiguration.UseMultiProcess";

namespace Debugger {
namespace Internal {

////////////////////////////////////////////////////////////////////////
//
// DebuggerRunConfigWidget
//
////////////////////////////////////////////////////////////////////////

class DebuggerRunConfigWidget : public ProjectExplorer::RunConfigWidget
{
    Q_OBJECT

public:
    explicit DebuggerRunConfigWidget(DebuggerRunConfigurationAspect *aspect);
    QString displayName() const { return tr("Debugger Settings"); }

private slots:
    void useCppDebuggerToggled(bool on);
    void useQmlDebuggerToggled(bool on);
    void qmlDebugServerPortChanged(int port);
    void useMultiProcessToggled(bool on);
    void update();

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

    connect(m_qmlDebuggerInfoLabel, SIGNAL(linkActivated(QString)),
            Core::HelpManager::instance(), SLOT(handleHelpRequest(QString)));
    connect(m_useQmlDebugger, SIGNAL(toggled(bool)),
            SLOT(useQmlDebuggerToggled(bool)));
    connect(m_useCppDebugger, SIGNAL(toggled(bool)),
            SLOT(useCppDebuggerToggled(bool)));
    connect(m_debugServerPort, SIGNAL(valueChanged(int)),
            SLOT(qmlDebugServerPortChanged(int)));
    connect(m_useMultiProcess, SIGNAL(toggled(bool)),
            SLOT(useMultiProcessToggled(bool)));

    QHBoxLayout *qmlLayout = new QHBoxLayout;
    qmlLayout->setMargin(0);
    qmlLayout->addWidget(m_useQmlDebugger);
    qmlLayout->addWidget(m_debugServerPortLabel);
    qmlLayout->addWidget(m_debugServerPort);
    qmlLayout->addWidget(m_qmlDebuggerInfoLabel);
    qmlLayout->addStretch();

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->addWidget(m_useCppDebugger);
    layout->addLayout(qmlLayout);
    layout->addWidget(m_useMultiProcess);
    setLayout(layout);

    connect(aspect->runConfiguration()->target()->project(), SIGNAL(projectLanguagesUpdated()),
            this, SLOT(update()));
    update();
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

void DebuggerRunConfigWidget::useCppDebuggerToggled(bool on)
{
    m_aspect->m_useCppDebugger = on
            ? DebuggerRunConfigurationAspect::EnabledLanguage
            : DebuggerRunConfigurationAspect::DisabledLanguage;
    if (!on && !m_useQmlDebugger->isChecked())
        m_useQmlDebugger->setChecked(true);
}

void DebuggerRunConfigWidget::useQmlDebuggerToggled(bool on)
{
    m_debugServerPort->setEnabled(on);
    m_debugServerPortLabel->setEnabled(on);

    m_aspect->m_useQmlDebugger = on
            ? DebuggerRunConfigurationAspect::EnabledLanguage
            : DebuggerRunConfigurationAspect::DisabledLanguage;
    if (!on && !m_useCppDebugger->isChecked())
        m_useCppDebugger->setChecked(true);
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
        ProjectExplorer::RunConfiguration *rc) :
    m_runConfiguration(rc),
    m_useCppDebugger(AutoEnabledLanguage),
    m_useQmlDebugger(AutoEnabledLanguage),
    m_qmlDebugServerPort(Constants::QML_DEFAULT_DEBUG_SERVER_PORT),
    m_useMultiProcess(false)
{
    ctor();
}

DebuggerRunConfigurationAspect::DebuggerRunConfigurationAspect(
        ProjectExplorer::RunConfiguration *runConfiguration,
        const DebuggerRunConfigurationAspect *other)
    : m_runConfiguration(runConfiguration),
      m_useCppDebugger(other->m_useCppDebugger),
      m_useQmlDebugger(other->m_useQmlDebugger),
      m_qmlDebugServerPort(other->m_qmlDebugServerPort),
      m_useMultiProcess(other->m_useMultiProcess)
{
    ctor();
}

ProjectExplorer::RunConfiguration *DebuggerRunConfigurationAspect::runConfiguration()
{
    return m_runConfiguration;
}

void DebuggerRunConfigurationAspect::setUseQmlDebugger(bool value)
{
    m_useQmlDebugger = value ? EnabledLanguage : DisabledLanguage;
    emit debuggersChanged();
}

void DebuggerRunConfigurationAspect::setUseCppDebugger(bool value)
{
    m_useCppDebugger = value ? EnabledLanguage : DisabledLanguage;
    emit debuggersChanged();
}

bool DebuggerRunConfigurationAspect::useCppDebugger() const
{
    if (m_useCppDebugger == DebuggerRunConfigurationAspect::AutoEnabledLanguage)
        return m_runConfiguration->target()->project()->projectLanguages().contains(
                    ProjectExplorer::Constants::LANG_CXX);
    return m_useCppDebugger == DebuggerRunConfigurationAspect::EnabledLanguage;
}

bool DebuggerRunConfigurationAspect::useQmlDebugger() const
{
    if (m_useQmlDebugger == DebuggerRunConfigurationAspect::AutoEnabledLanguage)
        return m_runConfiguration->target()->project()->projectLanguages().contains(
                    ProjectExplorer::Constants::LANG_QMLJS);
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
    ProjectExplorer::Kit *k = m_runConfiguration->target()->kit();
    ProjectExplorer::IDevice::ConstPtr dev = ProjectExplorer::DeviceKitInformation::device(k);
    if (dev.isNull())
        return false;
    return dev->canAutoDetectPorts();
}

QString DebuggerRunConfigurationAspect::displayName() const
{
    return tr("Debugger settings");
}

QVariantMap DebuggerRunConfigurationAspect::toMap() const
{
    QVariantMap map;
    map.insert(QLatin1String(USE_CPP_DEBUGGER_KEY), m_useCppDebugger == EnabledLanguage);
    map.insert(QLatin1String(USE_CPP_DEBUGGER_AUTO_KEY), m_useCppDebugger == AutoEnabledLanguage);
    map.insert(QLatin1String(USE_QML_DEBUGGER_KEY), m_useQmlDebugger == EnabledLanguage);
    map.insert(QLatin1String(USE_QML_DEBUGGER_AUTO_KEY), m_useQmlDebugger == AutoEnabledLanguage);
    map.insert(QLatin1String(QML_DEBUG_SERVER_PORT_KEY), m_qmlDebugServerPort);
    map.insert(QLatin1String(USE_MULTIPROCESS_KEY), m_useMultiProcess);
    return map;
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

DebuggerRunConfigurationAspect *DebuggerRunConfigurationAspect::clone(
        ProjectExplorer::RunConfiguration *parent) const
{
    return new DebuggerRunConfigurationAspect(parent, this);
}

ProjectExplorer::RunConfigWidget *DebuggerRunConfigurationAspect::createConfigurationWidget()
{
    return new Internal::DebuggerRunConfigWidget(this);
}

void DebuggerRunConfigurationAspect::ctor()
{
    connect(this, SIGNAL(debuggersChanged()),
            m_runConfiguration, SIGNAL(requestRunActionsUpdate()));
}

} // namespace Debugger


#include "debuggerrunconfigurationaspect.moc"
