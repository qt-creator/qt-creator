// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "customlanguagemodels.h"

#include "coreconstants.h"
#include "coreplugintr.h"
#include "dialogs/ioptionspage.h"

#include <utils/algorithm.h>
#include <utils/aspects.h>
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

class LanguageModelItem : public TreeItem
{
public:
    LanguageModelItem(const std::shared_ptr<BaseAspect> &model);

    bool hasModel(const std::shared_ptr<BaseAspect> &aspect) const { return aspect == m_model; }
    const CustomLanguageModel &model() const { return CustomLanguageModel::fromBaseAspect(m_model); }

private:
    QVariant data(int column, int role) const override;

    const std::shared_ptr<BaseAspect> m_model;
    QObject guard;
};

class LanguageModelsListModel : public TreeModel<TreeItem>
{
public:
    LanguageModelsListModel(AspectList &models);
    void sync();

private:
    void appendChild(const std::shared_ptr<BaseAspect> &aspect);

    AspectList &m_models;
};

class CustomLanguageModels : public AspectContainer
{
public:
    CustomLanguageModels();

    AspectList models{this};
    LanguageModelsListModel listModel{models};
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
    executable.setSettingsKey("Executable");
    executable.setDisplayName(Tr::tr("Executable"));
    executable.setLabelText(Tr::tr("Executable:"));
    executable.setExpectedKind(PathChooser::ExistingCommand);
    arguments.setSettingsKey("Arguments");
    arguments.setDisplayName(Tr::tr("Arguments"));
    arguments.setLabelText(Tr::tr("Arguments:"));
    arguments.setDisplayStyle(StringAspect::LineEditDisplay);

    using namespace Layouting;
    setLayouter([this] { return Form{name, br, executable, br, arguments}; });
}

LanguageModelsListModel::LanguageModelsListModel(AspectList &models) : m_models(models)
{
    sync();
    connect(&m_models, &BaseAspect::volatileValueChanged,
            this, &LanguageModelsListModel::sync);
    m_models.setItemAddedCallback([this](const std::shared_ptr<BaseAspect> &aspect) {
        appendChild(aspect);
    });
    m_models.setItemRemovedCallback([this](const std::shared_ptr<BaseAspect> &aspect) {
        for (int i = 0; i < rootItem()->childCount(); ++i) {
            if (static_cast<LanguageModelItem *>(rootItem()->childAt(i))->hasModel(aspect)) {
                rootItem()->removeChildAt(i);
                break;
            }
        }
    });
}

void LanguageModelsListModel::sync()
{
    clear();
    for (const auto &aspect : m_models.volatileItems())
        appendChild(aspect);
}

void LanguageModelsListModel::appendChild(const std::shared_ptr<BaseAspect> &aspect)
{
    rootItem()->appendChild(new LanguageModelItem(aspect));
}

LanguageModelItem::LanguageModelItem(const std::shared_ptr<BaseAspect> &model) : m_model(model)
{
    QObject::connect(model.get(), &BaseAspect::volatileValueChanged,
                     &guard, [this] { update(); });
}

QVariant LanguageModelItem::data(int column, int role) const
{
    if (column != 0)
        return {};
    if (role == Qt::DisplayRole)
        return CustomLanguageModel::fromBaseAspect(m_model).name.volatileValue();
    if (role == Qt::FontRole) {
        QFont f;
        f.setBold(m_model->isDirty());
        return f;
    }
    return {};
}

CustomLanguageModels::CustomLanguageModels()
{
    setAutoApply(false);
    setSettingsGroup("CustomLanguageModels");
    models.setCreateItemFunction([] { return std::make_shared<CustomLanguageModel>(); });
    models.setSettingsKey("ModelList");
    readSettings();
    listModel.sync();
    setLayouter([this] {
        // Widgets
        const auto view = new TreeView;
        view->setModel(&listModel);
        view->header()->hide();
        const auto addButton = new QPushButton(Tr::tr("Add"));
        const auto removeButton = new QPushButton(Tr::tr("Remove"));
        const auto configWidget = new QWidget;
        removeButton->setEnabled(false);
        connect(addButton, &QPushButton::clicked, this, [this] {
            const auto model = std::make_shared<CustomLanguageModel>();
            models.addItem(model);
            model->name.setVolatileValue(Tr::tr("<New model>"));
        });
        connect(removeButton, &QPushButton::clicked, this, [this, view] {
            QTC_ASSERT(view->currentIndex().isValid(), return);
            const auto item = static_cast<LanguageModelItem *>(
                listModel.itemForIndex(view->currentIndex()));
            QTC_ASSERT(item, return);
            for (const auto &l = models.volatileItems(); const auto &i : l) {
                if (item->hasModel(i)) {
                    models.removeItem(i);
                    break;
                }
            }
        });
        connect(view->selectionModel(), &QItemSelectionModel::currentChanged,
                this, [this, configWidget, removeButton](const QModelIndex &current) {
                    const bool hasCurrent = current.isValid();
                    removeButton->setEnabled(hasCurrent);
                    if (hasCurrent) {
                        const auto item = static_cast<LanguageModelItem *>(
                            listModel.itemForIndex(current));
                        QTC_ASSERT(item, return);
                        delete configWidget->layout();
                        item->model().layouter()().attachTo(configWidget);
                    }
                    configWidget->setVisible(hasCurrent);
                });

        // Layouts
        using namespace Layouting;
        return Column {
            Row {
                view,
                Column {
                    addButton,
                    removeButton,
                    st
                }
            },
            configWidget
        };
    });
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
