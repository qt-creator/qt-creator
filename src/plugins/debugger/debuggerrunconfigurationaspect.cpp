// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "debuggerrunconfigurationaspect.h"

#include "debuggertr.h"

#include <cppeditor/cppmodelmanager.h>

#include <coreplugin/helpmanager.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtbuildaspects.h>

#include <utils/detailswidget.h>
#include <utils/environment.h>
#include <utils/layoutbuilder.h>

#include <QDebug>
#include <QLabel>

using namespace ProjectExplorer;
using namespace Utils;

namespace Debugger {

/*!
    \class Debugger::DebuggerRunConfigurationAspect
*/

static bool isDisabled(TriStateAspect *aspect)
{
    QTC_ASSERT(aspect, return false);
    return aspect->value() == TriState::Disabled;
}

DebuggerRunConfigurationAspect::DebuggerRunConfigurationAspect(Target *target)
    : m_target(target)
{
    setId("DebuggerAspect");
    setDisplayName(Tr::tr("Debugger settings"));

    setConfigWidgetCreator([this] {
        Layouting::Grid builder;
        builder.addRow({m_cppAspect});
        auto info = new QLabel(
            Tr::tr("<a href=\""
                   "qthelp://org.qt-project.qtcreator/doc/creator-debugging-qml.html"
                   "\">What are the prerequisites?</a>"));
        builder.addRow({m_qmlAspect, info});
        builder.addRow({m_pythonAspect});
        connect(info, &QLabel::linkActivated, [](const QString &link) {
            Core::HelpManager::showHelpUrl(link);
        });
        builder.addRow({m_overrideStartupAspect});

        static const QString env = qtcEnvironmentVariable("QTC_DEBUGGER_MULTIPROCESS");
        if (env.toInt())
            builder.addRow({m_multiProcessAspect});

        auto details = new DetailsWidget;
        details->setState(DetailsWidget::Expanded);
        auto innerPane = new QWidget;
        details->setWidget(innerPane);
        builder.addItem(Layouting::noMargin);
        builder.attachTo(innerPane);

        const auto setSummaryText = [this, details] {
            const auto describe = [](const TriStateAspect &aspect, const QString &name) {
                if (aspect() == TriState::Enabled) {
                    //: %1 is C++, QML, or Python
                    return Tr::tr("Enable %1 debugger.").arg(name);
                }
                if (aspect() == TriState::Disabled) {
                    //: %1 is C++, QML, or Python
                    return Tr::tr("Disable %1 debugger.").arg(name);
                }
                //: %1 is C++, QML, or Python
                return Tr::tr("Try to determine need for %1 debugger.").arg(name);
            };

            details->setSummaryText(QStringList{
                describe(m_cppAspect, "C++"),
                describe(m_qmlAspect, "QML"),
                describe(m_pythonAspect, "Python"),
                m_overrideStartupAspect().isEmpty()
                                 ? Tr::tr("No additional startup commands.")
                                 : Tr::tr("Use additional startup commands.")
            }.join(" "));
        };
        setSummaryText();

        connect(&m_cppAspect, &BaseAspect::changed, details, setSummaryText);
        connect(&m_qmlAspect, &BaseAspect::changed, details, setSummaryText);
        connect(&m_pythonAspect, &BaseAspect::changed, details, setSummaryText);
        connect(&m_overrideStartupAspect, &BaseAspect::changed, details, setSummaryText);

        return details;
    });

    addDataExtractor(this, &DebuggerRunConfigurationAspect::useCppDebugger, &Data::useCppDebugger);
    addDataExtractor(this, &DebuggerRunConfigurationAspect::useQmlDebugger, &Data::useQmlDebugger);
    addDataExtractor(this, &DebuggerRunConfigurationAspect::usePythonDebugger, &Data::usePythonDebugger);
    addDataExtractor(this, &DebuggerRunConfigurationAspect::useMultiProcess, &Data::useMultiProcess);
    addDataExtractor(this, &DebuggerRunConfigurationAspect::overrideStartup, &Data::overrideStartup);

    m_cppAspect.setSettingsKey("RunConfiguration.UseCppDebugger");
    m_cppAspect.setLabelText(Tr::tr("C++ debugger:"));
    m_cppAspect.setOptionText(TriState::DefaultValue, Tr::tr("Automatic"));

    m_qmlAspect.setSettingsKey("RunConfiguration.UseQmlDebugger");
    m_qmlAspect.setLabelText(Tr::tr("QML debugger:"));
    m_qmlAspect.setOptionText(TriState::DefaultValue, Tr::tr("Automatic"));

    m_pythonAspect.setSettingsKey("RunConfiguration.UsePythonDebugger");
    m_pythonAspect.setLabelText(Tr::tr("Python debugger:"));
    m_pythonAspect.setOptionText(TriState::DefaultValue, Tr::tr("Automatic"));

    // Make sure at least one of the debuggers is set to be active.
    connect(&m_cppAspect, &TriStateAspect::changed, this, [this] {
        if (Utils::allOf({&m_cppAspect, &m_qmlAspect, &m_pythonAspect}, &isDisabled))
            m_qmlAspect.setValue(TriState::Default);
    });
    connect(&m_qmlAspect, &TriStateAspect::changed, this, [this] {
        if (Utils::allOf({&m_cppAspect, &m_qmlAspect, &m_pythonAspect}, &isDisabled))
            m_cppAspect.setValue(TriState::Default);
    });
    connect(&m_pythonAspect, &TriStateAspect::changed, this, [this] {
        if (Utils::allOf({&m_cppAspect, &m_qmlAspect, &m_pythonAspect}, &isDisabled))
            m_cppAspect.setValue(TriState::Default);
    });

    m_multiProcessAspect.setSettingsKey("RunConfiguration.UseMultiProcess");
    m_multiProcessAspect.setLabel(Tr::tr("Enable Debugging of Subprocesses"),
                                   BoolAspect::LabelPlacement::AtCheckBox);

    m_overrideStartupAspect.setSettingsKey("RunConfiguration.OverrideDebuggerStartup");
    m_overrideStartupAspect.setDisplayStyle(StringAspect::TextEditDisplay);
    m_overrideStartupAspect.setLabelText(Tr::tr("Additional startup commands:"));
}

DebuggerRunConfigurationAspect::~DebuggerRunConfigurationAspect() = default;

void DebuggerRunConfigurationAspect::setUseQmlDebugger(bool value)
{
    m_qmlAspect.setValue(value ? TriState::Enabled : TriState::Disabled);
}

bool DebuggerRunConfigurationAspect::useCppDebugger() const
{
    if (m_cppAspect() == TriState::Default)
        return m_target->project()->projectLanguages().contains(
                    ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    return m_cppAspect() == TriState::Enabled;
}

static bool projectHasQmlDefines(ProjectExplorer::Project *project)
{
    auto projectInfo = CppEditor::CppModelManager::projectInfo(project);
    if (!projectInfo) // we may have e.g. a Python project
        return false;
    return Utils::anyOf(projectInfo->projectParts(),
                        [](const CppEditor::ProjectPart::ConstPtr &part){
                            return Utils::anyOf(part->projectMacros, [](const Macro &macro){
                                return macro.key == "QT_DECLARATIVE_LIB"
                                       || macro.key == "QT_QUICK_LIB"
                                       || macro.key == "QT_QML_LIB";
                            });
                        });
}

bool DebuggerRunConfigurationAspect::useQmlDebugger() const
{
    if (m_qmlAspect() == TriState::Default) {
        const Core::Context languages = m_target->project()->projectLanguages();
        if (!languages.contains(ProjectExplorer::Constants::QMLJS_LANGUAGE_ID))
            return projectHasQmlDefines(m_target->project());

        // Try to find a build configuration to check whether qml debugging is enabled there
        if (BuildConfiguration *bc = m_target->activeBuildConfiguration()) {
            if (const auto aspect = bc->aspect<QtSupport::QmlDebuggingAspect>())
                return aspect->value() == TriState::Enabled;
        }

        return !languages.contains(ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    }
    return m_qmlAspect() == TriState::Enabled;
}

bool DebuggerRunConfigurationAspect::usePythonDebugger() const
{
    if (m_pythonAspect() == TriState::Default) {
        const Core::Context languages = m_target->project()->projectLanguages();
        return languages.contains(ProjectExplorer::Constants::PYTHON_LANGUAGE_ID);
    }
    return m_pythonAspect() == TriState::Enabled;
}

bool DebuggerRunConfigurationAspect::useMultiProcess() const
{
    return m_multiProcessAspect();
}

void DebuggerRunConfigurationAspect::setUseMultiProcess(bool value)
{
    m_multiProcessAspect.setValue(value);
}

QString DebuggerRunConfigurationAspect::overrideStartup() const
{
    return m_overrideStartupAspect();
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

void DebuggerRunConfigurationAspect::toMap(Store &map) const
{
    m_cppAspect.toMap(map);
    m_qmlAspect.toMap(map);
    m_pythonAspect.toMap(map);
    m_multiProcessAspect.toMap(map);
    m_overrideStartupAspect.toMap(map);

    // compatibility to old settings
    map.insert("RunConfiguration.UseCppDebuggerAuto", m_cppAspect() == TriState::Default);
    map.insert("RunConfiguration.UseQmlDebuggerAuto", m_qmlAspect() == TriState::Default);
}

void DebuggerRunConfigurationAspect::fromMap(const Store &map)
{
    m_cppAspect.fromMap(map);
    m_qmlAspect.fromMap(map);
    m_pythonAspect.fromMap(map);

    // respect old project settings
    if (map.value("RunConfiguration.UseCppDebuggerAuto", false).toBool())
        m_cppAspect.setValue(TriState::Default);
    if (map.value("RunConfiguration.UseQmlDebuggerAuto", false).toBool())
        m_qmlAspect.setValue(TriState::Default);

    m_multiProcessAspect.fromMap(map);
    m_overrideStartupAspect.fromMap(map);
}

} // namespace Debugger
