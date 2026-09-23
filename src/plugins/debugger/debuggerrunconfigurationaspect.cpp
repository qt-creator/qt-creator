// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "debuggerrunconfigurationaspect.h"

#include "debuggertr.h"

#include <cppeditor/cppmodelmanager.h>

#include <coreplugin/helpmanager.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/environmentkitaspect.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtbuildaspects.h>

#include <utils/detailswidget.h>
#include <utils/environment.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QLabel>

using namespace ProjectExplorer;
using namespace Utils;

namespace Debugger {

/*!
    \class Debugger::DebuggerRunConfigurationAspect
*/

enum class LanguageSelection {
    Automatic,
    Cpp,
    Qml,
    CppAndQml,
    CppAndQmlCombined,
    Python,
    CppAndPython,
    QmlAndPython,
    CppQmlAndPython
};

static const Key cppKey = "RunConfiguration.UseCppDebugger";
static const Key qmlKey = "RunConfiguration.UseQmlDebugger";
static const Key pythonKey = "RunConfiguration.UsePythonDebugger";
static const Key cppAutoKey = "RunConfiguration.UseCppDebuggerAuto";
static const Key qmlAutoKey = "RunConfiguration.UseQmlDebuggerAuto";

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

static bool autoUseQmlDebugger(BuildConfiguration *bc)
{
    const Core::Context languages = bc->project()->projectLanguages();
    if (!languages.contains(ProjectExplorer::Constants::QMLJS_LANGUAGE_ID))
        return projectHasQmlDefines(bc->project());

    // Try to find a build configuration to check whether qml debugging is enabled there
    if (const auto aspect = bc->aspect<QtSupport::QmlDebuggingAspect>())
        return aspect->value() == TriState::Enabled;

    return !languages.contains(ProjectExplorer::Constants::CXX_LANGUAGE_ID);
}

static bool autoUsePythonDebugger(BuildConfiguration *bc)
{
    return bc->project()->projectLanguages().contains(
        ProjectExplorer::Constants::PYTHON_LANGUAGE_ID);
}

static bool autoUseCppDebugger(BuildConfiguration *bc, bool otherDebuggerEnabled)
{
    if (bc->project()->projectLanguages().contains(ProjectExplorer::Constants::CXX_LANGUAGE_ID))
        return true;
    // If there is no other debugger, enable cpp debugger as a fallback to avoid leaving user
    // without any debugging support.
    return !otherDebuggerEnabled;
}

static bool hasParsedProject(BuildConfiguration *bc)
{
    BuildSystem * const buildSystem = bc->buildSystem();
    if (!buildSystem || !buildSystem->hasParsingData())
        return false;
    // The QML defines the mapping may consult come from the C++ code model, which parses on
    // its own. A project without C++ never gets that data, so only wait where it can arrive.
    Project * const project = bc->project();
    if (!project->projectLanguages().contains(ProjectExplorer::Constants::CXX_LANGUAGE_ID))
        return true;
    return bool(CppEditor::CppModelManager::projectInfo(project));
}

// Only a legacy setting that mixes automatic and explicit languages has to be resolved
// against the project contents. All-automatic amounts to "Automatic", all-explicit to the
// combination it names.
static bool needsProjectContents(TriState cpp, TriState qml, TriState python)
{
    const int automatic = int(cpp == TriState::Default) + int(qml == TriState::Default)
                          + int(python == TriState::Default);
    return automatic != 0 && automatic != 3;
}

static LanguageSelection languagesFor(bool cpp, bool qml, bool python)
{
    if (qml && python)
        return cpp ? LanguageSelection::CppQmlAndPython : LanguageSelection::QmlAndPython;
    if (python)
        return cpp ? LanguageSelection::CppAndPython : LanguageSelection::Python;
    if (qml)
        return cpp ? LanguageSelection::CppAndQml : LanguageSelection::Qml;
    return LanguageSelection::Cpp;
}

DebuggerRunConfigurationAspect::DebuggerRunConfigurationAspect(BuildConfiguration *bc)
    : m_buildConfiguration(bc)
{
    setId("DebuggerAspect");
    setDisplayName(Tr::tr("Debugger Settings"));

    setConfigWidgetCreator([this] {
        // The combo box cannot show a set of legacy per-language settings, so settle on the
        // item they amount to before the user gets to see anything. What they amount to
        // depends on the project contents, so an unparsed project has to be waited for: it
        // would answer "no C++" and drop that language for good.
        const auto settleLegacyLanguages = [this] {
            if (m_legacyLanguages
                && (!needsProjectContents(m_legacyLanguages->cpp, m_legacyLanguages->qml,
                                          m_legacyLanguages->python)
                    || hasParsedProject(m_buildConfiguration))) {
                setLanguages(languages());
            }
            // Until then the box would display "Automatic" while the legacy settings decide,
            // and offer to replace a selection it does not show.
            m_languagesAspect.setEnabled(!m_legacyLanguages);
        };
        settleLegacyLanguages();

        Layouting::Grid builder;
        auto info = new QLabel(
            Tr::tr("<a href=\""
                   "qthelp://org.qt-project.qtcreator/doc/creator-debugging-qml.html"
                   "\">What are the prerequisites?</a>"));
        builder.addRow({m_languagesAspect, info});
        connect(info, &QLabel::linkActivated, [](const QString &link) {
            Core::HelpManager::showHelpUrl(link);
        });
        const auto updateInfoVisibility = [this, info] { info->setVisible(useQmlDebugger()); };
        connect(&m_languagesAspect, &BaseAspect::changed, info, updateInfoVisibility);
        builder.addRow({m_overrideStartupAspect});

        static const QString env = qtcEnvironmentVariable("QTC_DEBUGGER_MULTIPROCESS");
        if (env.toInt())
            builder.addRow({m_multiProcessAspect});

        auto details = new DetailsWidget;
        details->setState(DetailsWidget::Expanded);
        auto innerPane = new QWidget;
        details->setWidget(innerPane);
        builder.setNoMargins();
        builder.attachTo(innerPane);

        // Showing the label any earlier would pop it up as a window of its own: it gets its
        // parent here.
        updateInfoVisibility();

        const auto setSummaryText = [this, details] {
            const QString languages = m_languagesAspect.value() == int(LanguageSelection::Automatic)
                ? Tr::tr("Determine the debuggers to use automatically.")
                //: %1 is a debugger combination, e.g. "C++ and QML (separate engines)"
                : Tr::tr("Debug %1.").arg(m_languagesAspect.stringValue());

            details->setSummaryText(QStringList{
                languages,
                m_overrideStartupAspect().isEmpty()
                                 ? Tr::tr("No additional startup commands.")
                                 : Tr::tr("Use additional startup commands.")
            }.join(" "));
        };
        setSummaryText();

        connect(&m_languagesAspect, &BaseAspect::changed, details, setSummaryText);
        connect(&m_overrideStartupAspect, &BaseAspect::changed, details, setSummaryText);

        if (BuildSystem * const buildSystem = m_buildConfiguration->buildSystem())
            connect(buildSystem, &BuildSystem::parsingFinished, details, settleLegacyLanguages);

        // The code model finishes separately, and the mapping may need its project parts.
        connect(CppEditor::CppModelManager::instance(),
                &CppEditor::CppModelManager::projectPartsUpdated,
                details, [this, settleLegacyLanguages](Project *project) {
            if (project == m_buildConfiguration->project())
                settleLegacyLanguages();
        });

        return details;
    });

    addDataExtractor(this, &DebuggerRunConfigurationAspect::useCppDebugger, &Data::useCppDebugger);
    addDataExtractor(this, &DebuggerRunConfigurationAspect::useQmlDebugger, &Data::useQmlDebugger);
    addDataExtractor(this, &DebuggerRunConfigurationAspect::usePythonDebugger, &Data::usePythonDebugger);
    addDataExtractor(this, &DebuggerRunConfigurationAspect::useCombinedEngine, &Data::useCombinedEngine);
    addDataExtractor(this, &DebuggerRunConfigurationAspect::useMultiProcess, &Data::useMultiProcess);
    addDataExtractor(this, &DebuggerRunConfigurationAspect::overrideStartup, &Data::overrideStartup);

    m_languagesAspect.setSettingsKey("RunConfiguration.DebuggerLanguages");
    m_languagesAspect.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    m_languagesAspect.setLabelText(Tr::tr("Debug:"));

    // languages() turns the selected index back into a LanguageSelection, so the items have
    // to be added in the enum's order.
    const auto addLanguageOption = [this](LanguageSelection selection, const QString &displayName,
                                  const QString &savedValue, const QString &toolTip = {}) {
        QTC_CHECK(m_languagesAspect.optionCount() == int(selection));
        m_languagesAspect.addOption({displayName, toolTip, savedValue});
    };
    addLanguageOption(LanguageSelection::Automatic, Tr::tr("Automatic"), "Automatic",
              Tr::tr("Derive the debuggers to use from the project contents."));
    addLanguageOption(LanguageSelection::Cpp, Tr::tr("C++ only"), "Cpp");
    addLanguageOption(LanguageSelection::Qml, Tr::tr("QML only"), "Qml");
    addLanguageOption(LanguageSelection::CppAndQml, Tr::tr("C++ and QML (separate engines)"),
              "CppAndQml");
    addLanguageOption(LanguageSelection::CppAndQmlCombined,
              Tr::tr("C++ and QML (combined engine, experimental)"), "CppAndQmlCombined",
              Tr::tr("Use a single engine for both languages. Only some debuggers support "
                     "this. The QTC_DEBUGGER_NATIVE_MIXED environment variable overrides "
                     "this setting."));
    addLanguageOption(LanguageSelection::Python, Tr::tr("Python only"), "Python");
    addLanguageOption(LanguageSelection::CppAndPython, Tr::tr("C++ and Python"), "CppAndPython");
    addLanguageOption(LanguageSelection::QmlAndPython, Tr::tr("QML and Python"), "QmlAndPython");
    addLanguageOption(LanguageSelection::CppQmlAndPython, Tr::tr("C++, QML, and Python"),
              "CppQmlAndPython");
    m_languagesAspect.setUseDataAsSavedValue();
    m_languagesAspect.setDefaultValue(int(LanguageSelection::Automatic));

    connect(&m_languagesAspect, &BaseAspect::changed, this, [this] {
        m_legacyLanguages.reset();
    });

    m_multiProcessAspect.setSettingsKey("RunConfiguration.UseMultiProcess");
    m_multiProcessAspect.setLabel(Tr::tr("Enable Debugging of Subprocesses"),
                                   BoolAspect::LabelPlacement::AtCheckBox);

    m_overrideStartupAspect.setSettingsKey("RunConfiguration.OverrideDebuggerStartup");
    m_overrideStartupAspect.setDisplayStyle(StringAspect::TextEditDisplay);
    m_overrideStartupAspect.setLabelText(Tr::tr("Additional startup commands:"));
}

DebuggerRunConfigurationAspect::~DebuggerRunConfigurationAspect() = default;

LanguageSelection DebuggerRunConfigurationAspect::languages() const
{
    return m_legacyLanguages ? legacyLanguages() : LanguageSelection(m_languagesAspect.value());
}

LanguageSelection DebuggerRunConfigurationAspect::legacyLanguages() const
{
    const LegacyLanguages legacy = *m_legacyLanguages;
    if (legacy.cpp == TriState::Default && legacy.qml == TriState::Default
            && legacy.python == TriState::Default) {
        return LanguageSelection::Automatic;
    }

    const bool qml = legacy.qml == TriState::Default
                         ? autoUseQmlDebugger(m_buildConfiguration)
                         : legacy.qml == TriState::Enabled;
    const bool python = legacy.python == TriState::Default
                            ? autoUsePythonDebugger(m_buildConfiguration)
                            : legacy.python == TriState::Enabled;
    const bool cpp = legacy.cpp == TriState::Default
                         ? autoUseCppDebugger(m_buildConfiguration, qml || python)
                         : legacy.cpp == TriState::Enabled;
    return languagesFor(cpp, qml, python);
}

void DebuggerRunConfigurationAspect::setLanguages(LanguageSelection languages)
{
    m_legacyLanguages.reset();
    m_languagesAspect.setValue(int(languages));
}

void DebuggerRunConfigurationAspect::setUseQmlDebugger(bool value)
{
    setLanguages(value ? LanguageSelection::CppAndQml : LanguageSelection::Cpp);
}

bool DebuggerRunConfigurationAspect::useCppDebugger() const
{
    switch (languages()) {
    case LanguageSelection::Cpp:
    case LanguageSelection::CppAndQml:
    case LanguageSelection::CppAndQmlCombined:
    case LanguageSelection::CppAndPython:
    case LanguageSelection::CppQmlAndPython:
        return true;
    case LanguageSelection::Qml:
    case LanguageSelection::Python:
    case LanguageSelection::QmlAndPython:
        return false;
    case LanguageSelection::Automatic:
        return autoUseCppDebugger(m_buildConfiguration,
                                  autoUseQmlDebugger(m_buildConfiguration)
                                      || autoUsePythonDebugger(m_buildConfiguration));
    }
    return false;
}

bool DebuggerRunConfigurationAspect::useQmlDebugger() const
{
    switch (languages()) {
    case LanguageSelection::Qml:
    case LanguageSelection::CppAndQml:
    case LanguageSelection::CppAndQmlCombined:
    case LanguageSelection::QmlAndPython:
    case LanguageSelection::CppQmlAndPython:
        return true;
    case LanguageSelection::Cpp:
    case LanguageSelection::Python:
    case LanguageSelection::CppAndPython:
        return false;
    case LanguageSelection::Automatic:
        return autoUseQmlDebugger(m_buildConfiguration);
    }
    return false;
}

bool DebuggerRunConfigurationAspect::usePythonDebugger() const
{
    switch (languages()) {
    case LanguageSelection::Python:
    case LanguageSelection::CppAndPython:
    case LanguageSelection::QmlAndPython:
    case LanguageSelection::CppQmlAndPython:
        return true;
    case LanguageSelection::Cpp:
    case LanguageSelection::Qml:
    case LanguageSelection::CppAndQml:
    case LanguageSelection::CppAndQmlCombined:
        return false;
    case LanguageSelection::Automatic:
        return autoUsePythonDebugger(m_buildConfiguration);
    }
    return false;
}

bool DebuggerRunConfigurationAspect::useCombinedEngine() const
{
    return languages() == LanguageSelection::CppAndQmlCombined;
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

void DebuggerRunConfigurationAspect::toMap(Store &map) const
{
    m_languagesAspect.toMap(map);
    m_multiProcessAspect.toMap(map);
    m_overrideStartupAspect.toMap(map);

    // compatibility to settings of Qt Creator 18 and earlier
    if (m_legacyLanguages) {
        // Not settled on an item yet, so write these back unchanged. What they amount to
        // depends on the project, and a guess made here would become the explicit setting,
        // dropping a language for good.
        map.insert(cppKey, m_legacyLanguages->cpp.toVariant());
        map.insert(qmlKey, m_legacyLanguages->qml.toVariant());
        map.insert(pythonKey, m_legacyLanguages->python.toVariant());
        map.insert(cppAutoKey, m_legacyLanguages->cpp == TriState::Default);
        map.insert(qmlAutoKey, m_legacyLanguages->qml == TriState::Default);
        return;
    }

    const bool automatic = languages() == LanguageSelection::Automatic;
    if (automatic) {
        const QVariant automaticValue = TriState::Default.toVariant();
        map.insert(cppKey, automaticValue);
        map.insert(qmlKey, automaticValue);
        map.insert(pythonKey, automaticValue);
    } else {
        const auto legacy = [](bool enabled) {
            return (enabled ? TriState::Enabled : TriState::Disabled).toVariant();
        };
        map.insert(cppKey, legacy(useCppDebugger()));
        map.insert(qmlKey, legacy(useQmlDebugger()));
        map.insert(pythonKey, legacy(usePythonDebugger()));
    }
    map.insert(cppAutoKey, automatic);
    map.insert(qmlAutoKey, automatic);
}

void DebuggerRunConfigurationAspect::fromMap(const Store &map)
{
    m_multiProcessAspect.fromMap(map);
    m_overrideStartupAspect.fromMap(map);

    m_legacyLanguages.reset();
    m_languagesAspect.fromMap(map);
    if (map.contains(m_languagesAspect.settingsKey()))
        return;

    // Settings of Qt Creator 18 and earlier. What they amount to depends on the project, which
    // is not necessarily set up at this point, so keep them until they are actually needed.
    const auto legacyValue = [&map](const Key &key) {
        return TriState::fromVariant(map.value(key, TriState::Default.toVariant()));
    };
    const auto legacyLanguage = [&map, &legacyValue](const Key &key, const Key &autoKey) {
        return map.value(autoKey, false).toBool() ? TriState::Default : legacyValue(key);
    };
    m_legacyLanguages = LegacyLanguages{legacyLanguage(cppKey, cppAutoKey),
                                        legacyLanguage(qmlKey, qmlAutoKey),
                                        legacyValue(pythonKey)};
}

} // namespace Debugger
