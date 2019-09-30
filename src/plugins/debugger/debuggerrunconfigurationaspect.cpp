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
#include <QDebug>
#include <QFormLayout>
#include <QLabel>
#include <QTextEdit>

const char USE_CPP_DEBUGGER_KEY[] = "RunConfiguration.UseCppDebugger";
const char USE_CPP_DEBUGGER_AUTO_KEY[] = "RunConfiguration.UseCppDebuggerAuto";
const char USE_QML_DEBUGGER_KEY[] = "RunConfiguration.UseQmlDebugger";
const char USE_QML_DEBUGGER_AUTO_KEY[] = "RunConfiguration.UseQmlDebuggerAuto";
const char USE_MULTIPROCESS_KEY[] = "RunConfiguration.UseMultiProcess";
const char OVERRIDE_STARTUP_KEY[] = "RunConfiguration.OverrideDebuggerStartup";

using namespace ProjectExplorer;

namespace Debugger {
namespace Internal {

////////////////////////////////////////////////////////////////////////
//
// DebuggerRunConfigWidget
//
////////////////////////////////////////////////////////////////////////

class DebuggerRunConfigWidget : public QWidget
{
    Q_DECLARE_TR_FUNCTIONS(Debugger::Internal::RunConfigWidget)

public:
    explicit DebuggerRunConfigWidget(DebuggerRunConfigurationAspect *aspect);

    void showEvent(QShowEvent *event) override;
    void update();

    void useCppDebuggerClicked(bool on);
    void useQmlDebuggerClicked(bool on);
    void useMultiProcessToggled(bool on);

public:
    DebuggerRunConfigurationAspect *m_aspect; // not owned

    QCheckBox *m_useCppDebugger;
    QCheckBox *m_useQmlDebugger;
    QLabel *m_qmlDebuggerInfoLabel;
    QLabel *m_overrideStartupLabel;
    QTextEdit *m_overrideStartupText;
    QCheckBox *m_useMultiProcess;
};

DebuggerRunConfigWidget::DebuggerRunConfigWidget(DebuggerRunConfigurationAspect *aspect)
{
    m_aspect = aspect;

    m_useCppDebugger = new QCheckBox(tr("Enable C++"), this);
    m_useQmlDebugger = new QCheckBox(tr("Enable QML"), this);

    m_qmlDebuggerInfoLabel = new QLabel(tr("<a href=\""
        "qthelp://org.qt-project.qtcreator/doc/creator-debugging-qml.html"
        "\">What are the prerequisites?</a>"));

    m_overrideStartupLabel = new QLabel(tr("Additional startup commands:"), this);
    m_overrideStartupText = new QTextEdit(this);

    static const QByteArray env = qgetenv("QTC_DEBUGGER_MULTIPROCESS");
    m_useMultiProcess =
        new QCheckBox(tr("Enable Debugging of Subprocesses"), this);
    m_useMultiProcess->setVisible(env.toInt());

    connect(m_qmlDebuggerInfoLabel, &QLabel::linkActivated,
            [](const QString &link) { Core::HelpManager::showHelpUrl(link); });
    connect(m_useQmlDebugger, &QAbstractButton::clicked,
            this, &DebuggerRunConfigWidget::useQmlDebuggerClicked);
    connect(m_useCppDebugger, &QAbstractButton::clicked,
            this, &DebuggerRunConfigWidget::useCppDebuggerClicked);
    connect(m_overrideStartupText, &QTextEdit::textChanged,
            this, [this] { m_aspect->d.overrideStartup = m_overrideStartupText->toPlainText(); });
    connect(m_useMultiProcess, &QAbstractButton::toggled,
            this, &DebuggerRunConfigWidget::useMultiProcessToggled);

    auto qmlLayout = new QHBoxLayout;
    qmlLayout->setContentsMargins(0, 0, 0, 0);
    qmlLayout->addWidget(m_useQmlDebugger);
    qmlLayout->addWidget(m_qmlDebuggerInfoLabel);
    qmlLayout->addStretch();

    auto layout = new QFormLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addRow(m_useCppDebugger);
    layout->addRow(qmlLayout);
    layout->addRow(m_overrideStartupLabel, m_overrideStartupText);
    layout->addRow(m_useMultiProcess);
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

    QWidget::showEvent(event);
}

void DebuggerRunConfigWidget::update()
{
    m_useCppDebugger->setChecked(m_aspect->useCppDebugger());
    m_useQmlDebugger->setChecked(m_aspect->useQmlDebugger());

    m_useMultiProcess->setChecked(m_aspect->useMultiProcess());

    m_overrideStartupText->setText(m_aspect->overrideStartup());
}

void DebuggerRunConfigWidget::useCppDebuggerClicked(bool on)
{
    m_aspect->d.useCppDebugger = on ? EnabledLanguage : DisabledLanguage;
    if (!on && !m_useQmlDebugger->isChecked()) {
        m_useQmlDebugger->setChecked(true);
        useQmlDebuggerClicked(true);
    }
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

DebuggerRunConfigurationAspect::DebuggerRunConfigurationAspect(Target *target)
    : m_target(target)
{
    setId("DebuggerAspect");
    setDisplayName(tr("Debugger settings"));
    setConfigWidgetCreator([this] { return new Internal::DebuggerRunConfigWidget(this); });
}

void DebuggerRunConfigurationAspect::setUseQmlDebugger(bool value)
{
    d.useQmlDebugger = value ? EnabledLanguage : DisabledLanguage;
}

bool DebuggerRunConfigurationAspect::useCppDebugger() const
{
    if (d.useCppDebugger == AutoEnabledLanguage)
        return m_target->project()->projectLanguages().contains(
                    ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    return d.useCppDebugger == EnabledLanguage;
}

bool DebuggerRunConfigurationAspect::useQmlDebugger() const
{
    if (d.useQmlDebugger == AutoEnabledLanguage) {
        const Core::Context languages = m_target->project()->projectLanguages();
        if (!languages.contains(ProjectExplorer::Constants::QMLJS_LANGUAGE_ID))
            return false;

        //
        // Try to find a build step (qmake) to check whether qml debugging is enabled there
        // (Using the Qt metatype system to avoid a hard qt4projectmanager dependency)
        //
        if (BuildConfiguration *bc = m_target->activeBuildConfiguration()) {
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

bool DebuggerRunConfigurationAspect::useMultiProcess() const
{
    return d.useMultiProcess;
}

void DebuggerRunConfigurationAspect::setUseMultiProcess(bool value)
{
    d.useMultiProcess = value;
}

QString DebuggerRunConfigurationAspect::overrideStartup() const
{
    return d.overrideStartup;
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
    map.insert(USE_CPP_DEBUGGER_KEY, d.useCppDebugger == EnabledLanguage);
    map.insert(USE_CPP_DEBUGGER_AUTO_KEY, d.useCppDebugger == AutoEnabledLanguage);
    map.insert(USE_QML_DEBUGGER_KEY, d.useQmlDebugger == EnabledLanguage);
    map.insert(USE_QML_DEBUGGER_AUTO_KEY, d.useQmlDebugger == AutoEnabledLanguage);
    map.insert(USE_MULTIPROCESS_KEY, d.useMultiProcess);
    map.insert(OVERRIDE_STARTUP_KEY, d.overrideStartup);
}

void DebuggerRunConfigurationAspect::fromMap(const QVariantMap &map)
{
    if (map.value(USE_CPP_DEBUGGER_AUTO_KEY, false).toBool()) {
        d.useCppDebugger = AutoEnabledLanguage;
    } else {
        bool useCpp = map.value(USE_CPP_DEBUGGER_KEY, false).toBool();
        d.useCppDebugger = useCpp ? EnabledLanguage : DisabledLanguage;
    }
    if (map.value(USE_QML_DEBUGGER_AUTO_KEY, false).toBool()) {
        d.useQmlDebugger = AutoEnabledLanguage;
    } else {
        bool useQml = map.value(USE_QML_DEBUGGER_KEY, false).toBool();
        d.useQmlDebugger = useQml ? EnabledLanguage : DisabledLanguage;
    }
    d.useMultiProcess = map.value(USE_MULTIPROCESS_KEY, false).toBool();
    d.overrideStartup = map.value(OVERRIDE_STARTUP_KEY).toString();
}

} // namespace Debugger
