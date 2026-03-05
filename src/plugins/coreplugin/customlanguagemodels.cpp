// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "customlanguagemodels.h"

#include "coreconstants.h"
#include "coreplugintr.h"
#include "dialogs/ioptionspage.h"

#include <utils/algorithm.h>
#include <utils/aspects.h>
#include <utils/guiutils.h>
#include <utils/itemviews.h>
#include <utils/layoutbuilder.h>
#include <utils/macroexpander.h>
#include <utils/treemodel.h>

#include <QHeaderView>
#include <QLayout>
#include <QPushButton>

using namespace Utils;

namespace Core {
namespace {

class CustomLanguageModel : public AspectContainer
{
public:
    static CustomLanguageModel &fromBaseAspect(BaseAspect &aspect);
    static const CustomLanguageModel &fromBaseAspect(const std::shared_ptr<BaseAspect> &aspect);

    CustomLanguageModel();

    CommandLine commandLine();

    StringAspect name{this};
    FilePathAspect executable{this};
    StringAspect arguments{this};
};

class CustomLanguageModels : public AspectContainer
{
public:
    CustomLanguageModels();

    AspectList models{this};
};

CustomLanguageModels &customLanguageModels()
{
    static CustomLanguageModels models;
    return models;
}

class CustomLanguageModelsSettingsPage final : public Core::IOptionsPage
{
public:
    CustomLanguageModelsSettingsPage()
    {
        setId("AI.CustomModels");
        setDisplayName(Tr::tr("Custom Language Models"));
        setCategory(Constants::SETTINGS_CATEGORY_AI);
        setSettingsProvider([] { return &customLanguageModels(); });
    }
};

CommandLine CustomLanguageModel::commandLine()
{
    return CommandLine(executable.effectiveBinary(), arguments(), CommandLine::Raw);
}

CustomLanguageModel &CustomLanguageModel::fromBaseAspect(BaseAspect &aspect)
{
    return static_cast<CustomLanguageModel &>(aspect);
}

const CustomLanguageModel &CustomLanguageModel::fromBaseAspect(
    const std::shared_ptr<BaseAspect> &aspect)
{
    return fromBaseAspect(*aspect);
}

CustomLanguageModel::CustomLanguageModel()
{
    setSettingsKey("CustomLanguageModel");
    name.setSettingsKey("Name");
    name.setDisplayName(Tr::tr("Name"));
    name.setDisplayStyle(StringAspect::LineEditDisplay);
    name.setLabelText(Tr::tr("Name:"));
    name.setValue(Tr::tr("<New model>"));

    executable.setSettingsKey("Executable");
    executable.setDisplayName(Tr::tr("Executable"));
    executable.setLabelText(Tr::tr("Executable:"));
    executable.setExpectedKind(PathChooser::ExistingCommand);

    arguments.setSettingsKey("Arguments");
    arguments.setDisplayName(Tr::tr("Arguments"));
    arguments.setLabelText(Tr::tr("Arguments:"));
    arguments.setDisplayStyle(StringAspect::LineEditDisplay);

    using namespace Layouting;
    setLayouter([this] { return Form { name, br, executable, br, arguments }; });
}

CustomLanguageModels::CustomLanguageModels()
{
    setAutoApply(false);
    setSettingsGroup("CustomLanguageModels");
    models.setCreateItemFunction([] { return std::make_shared<CustomLanguageModel>(); });
    models.setSettingsKey("ModelList");
    models.setDisplayStyle(AspectList::DisplayStyle::ListViewWithDetails);
    models.listViewDisplayCallback = [](CustomLanguageModel *aspect) {
        return aspect->name.volatileValue();
    };

    setLayouter([this] {
        using namespace Layouting;
        return Column { models };
    });

    readSettings();
}

} // namespace

const QStringList availableLanguageModels()
{
    return Utils::transform(
        customLanguageModels().models.items(), [](const std::shared_ptr<BaseAspect> &aspect) {
            return CustomLanguageModel::fromBaseAspect(aspect).name();
        });
}

CommandLine commandLineForLanguageModel(const QString &model)
{
    for (const auto &l = customLanguageModels().models.items(); const auto &i : l) {
        if (auto &m = CustomLanguageModel::fromBaseAspect(*i); m.name() == model)
            return m.commandLine();
    }
    return {};
}

BaseAspect &customLanguageModelsContext()
{
    return customLanguageModels();
}

namespace Internal {

void setupCustomLanguageModels()
{
    (void) customLanguageModels();
    static const CustomLanguageModelsSettingsPage settingsPage;
}

} // namespace Internal
} // namespace Core
