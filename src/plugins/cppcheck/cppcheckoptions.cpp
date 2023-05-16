// Copyright (C) 2018 Sergey Morozov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppcheckoptions.h"

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

#include <coreplugin/icore.h>

#include <debugger/analyzer/analyzericons.h>
#include <debugger/debuggertr.h>

using namespace Utils;

namespace Cppcheck::Internal {

CppcheckOptions::CppcheckOptions()
{
    setId(Constants::OPTIONS_PAGE_ID);
    setDisplayName(Tr::tr("Cppcheck"));
    setCategory("T.Analyzer");
    setDisplayCategory(::Debugger::Tr::tr("Analyzer"));
    setCategoryIconPath(Analyzer::Icons::SETTINGSCATEGORY_ANALYZER);
    setSettingsGroup("Cppcheck");

    registerAspect(&binary);
    binary.setSettingsKey("binary");
    binary.setDisplayStyle(StringAspect::PathChooserDisplay);
    binary.setExpectedKind(PathChooser::ExistingCommand);
    binary.setCommandVersionArguments({"--version"});
    binary.setLabelText(Tr::tr("Binary:"));
    if (HostOsInfo::isAnyUnixHost()) {
        binary.setDefaultValue("cppcheck");
    } else {
        FilePath programFiles = FilePath::fromUserInput(qtcEnvironmentVariable("PROGRAMFILES"));
        if (programFiles.isEmpty())
            programFiles = "C:/Program Files";
        binary.setDefaultValue(programFiles.pathAppended("Cppcheck/cppcheck.exe").toString());
    }

    registerAspect(&warning);
    warning.setSettingsKey("warning");
    warning.setDefaultValue(true);
    warning.setLabelText(Tr::tr("Warnings"));

    registerAspect(&style);
    style.setSettingsKey("style");
    style.setDefaultValue(true);
    style.setLabelText(Tr::tr("Style"));

    registerAspect(&performance);
    performance.setSettingsKey("performance");
    performance.setDefaultValue(true);
    performance.setLabelText(Tr::tr("Performance"));

    registerAspect(&portability);
    portability.setSettingsKey("portability");
    portability.setDefaultValue(true);
    portability.setLabelText(Tr::tr("Portability"));

    registerAspect(&information);
    information.setSettingsKey("information");
    information.setDefaultValue(true);
    information.setLabelText(Tr::tr("Information"));

    registerAspect(&unusedFunction);
    unusedFunction.setSettingsKey("unusedFunction");
    unusedFunction.setLabelText(Tr::tr("Unused functions"));
    unusedFunction.setToolTip(Tr::tr("Disables multithreaded check."));

    registerAspect(&missingInclude);
    missingInclude.setSettingsKey("missingInclude");
    missingInclude.setLabelText(Tr::tr("Missing includes"));

    registerAspect(&inconclusive);
    inconclusive.setSettingsKey("inconclusive");
    inconclusive.setLabelText(Tr::tr("Inconclusive errors"));

    registerAspect(&forceDefines);
    forceDefines.setSettingsKey("forceDefines");
    forceDefines.setLabelText(Tr::tr("Check all define combinations"));

    registerAspect(&customArguments);
    customArguments.setSettingsKey("customArguments");
    customArguments.setDisplayStyle(StringAspect::LineEditDisplay);
    customArguments.setLabelText(Tr::tr("Custom arguments:"));

    registerAspect(&ignoredPatterns);
    ignoredPatterns.setSettingsKey("ignoredPatterns");
    ignoredPatterns.setDisplayStyle(StringAspect::LineEditDisplay);
    ignoredPatterns.setLabelText(Tr::tr("Ignored file patterns:"));
    ignoredPatterns.setToolTip(Tr::tr("Comma-separated wildcards of full file paths. "
                                      "Files still can be checked if others include them."));

    registerAspect(&showOutput);
    showOutput.setSettingsKey("showOutput");
    showOutput.setLabelText(Tr::tr("Show raw output"));

    registerAspect(&addIncludePaths);
    addIncludePaths.setSettingsKey("addIncludePaths");
    addIncludePaths.setLabelText(Tr::tr("Add include paths"));
    addIncludePaths.setToolTip(Tr::tr("Can find missing includes but makes "
                                      "checking slower. Use only when needed."));

    registerAspect(&guessArguments);
    guessArguments.setSettingsKey("guessArguments");
    guessArguments.setDefaultValue(true);
    guessArguments.setLabelText(Tr::tr("Calculate additional arguments"));
    guessArguments.setToolTip(Tr::tr("Like C++ standard and language."));

    setLayouter(layouter());

    readSettings();
}

std::function<void(QWidget *widget)> CppcheckOptions::layouter()
{
    return [this](QWidget *widget) {
        using namespace Layouting;
        Form {
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
        }.attachTo(widget);
    };
}

} // Cppcheck::Internal
