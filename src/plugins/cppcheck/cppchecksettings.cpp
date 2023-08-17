// Copyright (C) 2018 Sergey Morozov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppchecksettings.h"

#include "cppcheckconstants.h"
#include "cppchecktool.h"
#include "cppchecktr.h"
#include "cppchecktrigger.h"

#include <utils/environment.h>
#include <utils/flowlayout.h>
#include <utils/hostosinfo.h>
#include <utils/pathchooser.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>
#include <utils/variablechooser.h>

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>

#include <debugger/analyzer/analyzericons.h>
#include <debugger/debuggertr.h>

using namespace Utils;

namespace Cppcheck::Internal {

CppcheckSettings &settings()
{
    static CppcheckSettings theSettings;
    return theSettings;
}

CppcheckSettings::CppcheckSettings()
{
    setSettingsGroup("Cppcheck");
    setAutoApply(false);

    binary.setSettingsKey("binary");
    binary.setExpectedKind(PathChooser::ExistingCommand);
    binary.setCommandVersionArguments({"--version"});
    binary.setLabelText(Tr::tr("Binary:"));
    if (HostOsInfo::isAnyUnixHost()) {
        binary.setDefaultValue("cppcheck");
    } else {
        QString programFiles = qtcEnvironmentVariable("PROGRAMFILES");
        if (programFiles.isEmpty())
            programFiles = "C:/Program Files";
        binary.setDefaultValue(programFiles + "/Cppcheck/cppcheck.exe");
    }

    warning.setSettingsKey("warning");
    warning.setDefaultValue(true);
    warning.setLabelText(Tr::tr("Warnings"));

    style.setSettingsKey("style");
    style.setDefaultValue(true);
    style.setLabelText(Tr::tr("Style"));

    performance.setSettingsKey("performance");
    performance.setDefaultValue(true);
    performance.setLabelText(Tr::tr("Performance"));

    portability.setSettingsKey("portability");
    portability.setDefaultValue(true);
    portability.setLabelText(Tr::tr("Portability"));

    information.setSettingsKey("information");
    information.setDefaultValue(true);
    information.setLabelText(Tr::tr("Information"));

    unusedFunction.setSettingsKey("unusedFunction");
    unusedFunction.setLabelText(Tr::tr("Unused functions"));
    unusedFunction.setToolTip(Tr::tr("Disables multithreaded check."));

    missingInclude.setSettingsKey("missingInclude");
    missingInclude.setLabelText(Tr::tr("Missing includes"));

    inconclusive.setSettingsKey("inconclusive");
    inconclusive.setLabelText(Tr::tr("Inconclusive errors"));

    forceDefines.setSettingsKey("forceDefines");
    forceDefines.setLabelText(Tr::tr("Check all define combinations"));

    customArguments.setSettingsKey("customArguments");
    customArguments.setDisplayStyle(StringAspect::LineEditDisplay);
    customArguments.setLabelText(Tr::tr("Custom arguments:"));

    ignoredPatterns.setSettingsKey("ignoredPatterns");
    ignoredPatterns.setDisplayStyle(StringAspect::LineEditDisplay);
    ignoredPatterns.setLabelText(Tr::tr("Ignored file patterns:"));
    ignoredPatterns.setToolTip(Tr::tr("Comma-separated wildcards of full file paths. "
                                      "Files still can be checked if others include them."));

    showOutput.setSettingsKey("showOutput");
    showOutput.setLabelText(Tr::tr("Show raw output"));

    addIncludePaths.setSettingsKey("addIncludePaths");
    addIncludePaths.setLabelText(Tr::tr("Add include paths"));
    addIncludePaths.setToolTip(Tr::tr("Can find missing includes but makes "
                                      "checking slower. Use only when needed."));

    guessArguments.setSettingsKey("guessArguments");
    guessArguments.setDefaultValue(true);
    guessArguments.setLabelText(Tr::tr("Calculate additional arguments"));
    guessArguments.setToolTip(Tr::tr("Like C++ standard and language."));

    setLayouter(layouter());

    readSettings();
}

std::function<Layouting::LayoutItem()> CppcheckSettings::layouter()
{
    return [this] {
        using namespace Layouting;
        return Form {
            binary, br,
            Tr::tr("Checks:"), Flow {
                warning,
                style,
                performance,
                portability,
                information,
                unusedFunction,
                missingInclude
            }, br,
            customArguments, br,
            ignoredPatterns, br,
            Flow {
                inconclusive,
                forceDefines,
                showOutput,
                addIncludePaths,
                guessArguments
            }
        };
    };
}

class CppCheckSettingsPage final : public Core::IOptionsPage
{
public:
    CppCheckSettingsPage()
    {
        setId(Constants::OPTIONS_PAGE_ID);
        setDisplayName(Tr::tr("Cppcheck"));
        setCategory("T.Analyzer");
        setDisplayCategory(::Debugger::Tr::tr("Analyzer"));
        setCategoryIconPath(Analyzer::Icons::SETTINGSCATEGORY_ANALYZER);
        setSettingsProvider([] { return &settings(); });
    }
};

const CppCheckSettingsPage settingsPage;

} // Cppcheck::Internal
