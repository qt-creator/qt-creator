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

#include <coreplugin/helpmanager.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtbuildaspects.h>

#include <QCheckBox>
#include <QDebug>
#include <QFormLayout>
#include <QLabel>
#include <QTextEdit>

using namespace ProjectExplorer;
using namespace Debugger::Internal;

namespace Debugger {
namespace Internal {

enum DebuggerLanguageStatus {
    DisabledLanguage = 0,
    EnabledLanguage,
    AutoEnabledLanguage
};

class DebuggerLanguageAspect : public ProjectConfigurationAspect
{
public:
    DebuggerLanguageAspect() = default;

    void addToLayout(LayoutBuilder &builder) override;

    bool value() const;
    void setValue(bool val);

    void setAutoSettingsKey(const QString &settingsKey);
    void setLabel(const QString &label);
    void setInfoLabelText(const QString &text) { m_infoLabelText = text; }

    void setClickCallBack(const std::function<void (bool)> &clickCallBack)
    {
        m_clickCallBack = clickCallBack;
    }

    void fromMap(const QVariantMap &map) override;
    void toMap(QVariantMap &map) const override;

public:
    DebuggerLanguageStatus m_value = AutoEnabledLanguage;
    bool m_defaultValue = false;
    QString m_label;
    QString m_infoLabelText;
    QPointer<QCheckBox> m_checkBox; // Owned by configuration widget
    QPointer<QLabel> m_infoLabel; // Owned by configuration widget
    QString m_autoSettingsKey;

    std::function<void(bool)> m_clickCallBack;
};

void DebuggerLanguageAspect::addToLayout(LayoutBuilder &builder)
{
    QTC_CHECK(!m_checkBox);
    m_checkBox = new QCheckBox(m_label);
    m_checkBox->setChecked(m_value);

    QTC_CHECK(m_clickCallBack);
    connect(m_checkBox, &QAbstractButton::clicked, this, m_clickCallBack, Qt::QueuedConnection);

    connect(m_checkBox.data(), &QAbstractButton::clicked, this, [this] {
        m_value = m_checkBox->isChecked() ? EnabledLanguage : DisabledLanguage;
        emit changed();
    });
    builder.addItem(QString());
    builder.addItem(m_checkBox.data());

    if (!m_infoLabelText.isEmpty()) {
        QTC_CHECK(!m_infoLabel);
        m_infoLabel = new QLabel(m_infoLabelText);
        connect(m_infoLabel, &QLabel::linkActivated, [](const QString &link) {
            Core::HelpManager::showHelpUrl(link);
        });
        builder.addItem(m_infoLabel.data());
    }
}

void DebuggerLanguageAspect::setAutoSettingsKey(const QString &settingsKey)
{
    m_autoSettingsKey = settingsKey;
}

void DebuggerLanguageAspect::fromMap(const QVariantMap &map)
{
    const bool val = map.value(settingsKey(), false).toBool();
    const bool autoVal = map.value(m_autoSettingsKey, false).toBool();
    m_value = autoVal ? AutoEnabledLanguage : val ? EnabledLanguage : DisabledLanguage;
}

void DebuggerLanguageAspect::toMap(QVariantMap &data) const
{
    data.insert(settingsKey(), m_value == EnabledLanguage);
    data.insert(m_autoSettingsKey, m_value == AutoEnabledLanguage);
}


bool DebuggerLanguageAspect::value() const
{
    return m_value;
}

void DebuggerLanguageAspect::setValue(bool value)
{
    m_value = value ? EnabledLanguage : DisabledLanguage;
    if (m_checkBox)
        m_checkBox->setChecked(m_value);
}

void DebuggerLanguageAspect::setLabel(const QString &label)
{
    m_label = label;
}

} // Internal

/*!
    \class Debugger::DebuggerRunConfigurationAspect
*/

DebuggerRunConfigurationAspect::DebuggerRunConfigurationAspect(Target *target)
    : m_target(target)
{
    setId("DebuggerAspect");
    setDisplayName(tr("Debugger settings"));

    setConfigWidgetCreator([this] {
        QWidget *w = new QWidget;
        LayoutBuilder builder(w);
        m_cppAspect->addToLayout(builder);
        m_qmlAspect->addToLayout(builder.startNewRow());
        m_overrideStartupAspect->addToLayout(builder.startNewRow());

        static const QByteArray env = qgetenv("QTC_DEBUGGER_MULTIPROCESS");
        if (env.toInt())
            m_multiProcessAspect->addToLayout(builder.startNewRow());

        return w;
    });

    m_cppAspect = new DebuggerLanguageAspect;
    m_cppAspect->setLabel(tr("Enable C++"));
    m_cppAspect->setSettingsKey("RunConfiguration.UseCppDebugger");
    m_cppAspect->setAutoSettingsKey("RunConfiguration.UseCppDebuggerAuto");

    m_qmlAspect = new DebuggerLanguageAspect;
    m_qmlAspect->setLabel(tr("Enable QML"));
    m_qmlAspect->setSettingsKey("RunConfiguration.UseQmlDebugger");
    m_qmlAspect->setAutoSettingsKey("RunConfiguration.UseQmlDebuggerAuto");
    m_qmlAspect->setInfoLabelText(tr("<a href=\""
        "qthelp://org.qt-project.qtcreator/doc/creator-debugging-qml.html"
        "\">What are the prerequisites?</a>"));

    // Make sure at least one of the debuggers is set to be active.
    m_cppAspect->setClickCallBack([this](bool on) {
        if (!on && !m_qmlAspect->value())
            m_qmlAspect->setValue(true);
    });
    m_qmlAspect->setClickCallBack([this](bool on) {
        if (!on && !m_cppAspect->value())
            m_cppAspect->setValue(true);
    });

    m_multiProcessAspect = new BaseBoolAspect;
    m_multiProcessAspect->setSettingsKey("RunConfiguration.UseMultiProcess");
    m_multiProcessAspect->setLabel(tr("Enable Debugging of Subprocesses"),
                                   BaseBoolAspect::LabelPlacement::AtCheckBox);

    m_overrideStartupAspect = new BaseStringAspect;
    m_overrideStartupAspect->setSettingsKey("RunConfiguration.OverrideDebuggerStartup");
    m_overrideStartupAspect->setDisplayStyle(BaseStringAspect::TextEditDisplay);
    m_overrideStartupAspect->setLabelText(tr("Additional startup commands:"));
}

DebuggerRunConfigurationAspect::~DebuggerRunConfigurationAspect()
{
    delete m_cppAspect;
    delete m_qmlAspect;
    delete m_multiProcessAspect;
    delete m_overrideStartupAspect;
}

void DebuggerRunConfigurationAspect::setUseQmlDebugger(bool value)
{
    m_qmlAspect->setValue(value);
}

bool DebuggerRunConfigurationAspect::useCppDebugger() const
{
    if (m_cppAspect->m_value == AutoEnabledLanguage)
        return m_target->project()->projectLanguages().contains(
                    ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    return m_cppAspect->m_value == EnabledLanguage;
}

bool DebuggerRunConfigurationAspect::useQmlDebugger() const
{
    if (m_qmlAspect->m_value == AutoEnabledLanguage) {
        const Core::Context languages = m_target->project()->projectLanguages();
        if (!languages.contains(ProjectExplorer::Constants::QMLJS_LANGUAGE_ID))
            return false;

        //
        // Try to find a build configuration to check whether qml debugging is enabled there
        if (BuildConfiguration *bc = m_target->activeBuildConfiguration()) {
            const auto aspect = bc->aspect<QtSupport::QmlDebuggingAspect>();
            return aspect && aspect->setting() == TriState::Enabled;
        }

        return !languages.contains(ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    }
    return m_qmlAspect->m_value == EnabledLanguage;
}

bool DebuggerRunConfigurationAspect::useMultiProcess() const
{
    return m_multiProcessAspect->value();
}

void DebuggerRunConfigurationAspect::setUseMultiProcess(bool value)
{
    m_multiProcessAspect->setValue(value);
}

QString DebuggerRunConfigurationAspect::overrideStartup() const
{
    return m_overrideStartupAspect->value();
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
    m_cppAspect->toMap(map);
    m_qmlAspect->toMap(map);
    m_multiProcessAspect->toMap(map);
    m_overrideStartupAspect->toMap(map);
}

void DebuggerRunConfigurationAspect::fromMap(const QVariantMap &map)
{
    m_cppAspect->fromMap(map);
    m_qmlAspect->fromMap(map);
    m_multiProcessAspect->fromMap(map);
    m_overrideStartupAspect->fromMap(map);
}

} // namespace Debugger
