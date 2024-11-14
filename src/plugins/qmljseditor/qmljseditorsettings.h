// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

#include <projectexplorer/project.h>

#include <utils/filepath.h>

#include <QMutex>
#include <QObject>
#include <QVersionNumber>

namespace QmlJSEditor::Internal {

class QmlJsEditingSettings final : public Utils::AspectContainer
{
public:
    QmlJsEditingSettings();

    QString defaultFormatCommand() const;

    Utils::BoolAspect enableContextPane{this};
    Utils::BoolAspect pinContextPane{this};
    Utils::BoolAspect autoFormatOnSave{this};
    Utils::BoolAspect autoFormatOnlyCurrentProject{this};
    Utils::BoolAspect foldAuxData{this};
    Utils::BoolAspect useCustomFormatCommand{this};
    Utils::BoolAspect useCustomAnalyzer{this};
    Utils::SelectionAspect uiQmlOpenMode{this};
    Utils::StringAspect formatCommand{this};
    Utils::StringAspect formatCommandOptions{this};
    Utils::IntegersAspect disabledMessages{this};
    Utils::IntegersAspect disabledMessagesForNonQuickUi{this};
};

QmlJsEditingSettings &settings();

class QmlJsEditingSettingsPage : public Core::IOptionsPage
{
public:
    QmlJsEditingSettingsPage();
};

class ProjectSettings : public Utils::AspectContainer
{
public:
    ProjectSettings(ProjectExplorer::Project *project);

    Utils::BoolAspect useQmlls{this};
    Utils::BoolAspect useGlobalSettings{this};

    void save(ProjectExplorer::Project *project);
};

void setupQmlJsEditingProjectPanel();

} // QmlJSEditor::Internal
