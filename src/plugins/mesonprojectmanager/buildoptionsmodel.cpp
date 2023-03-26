// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "buildoptionsmodel.h"

#include "arrayoptionlineedit.h"
#include "mesonprojectmanagertr.h"

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QStyledItemDelegate>
#include <QTextEdit>

namespace MesonProjectManager {
namespace Internal {

// this could be relaxed once we have something able to link reliably meson build type
// to QTC build type and update it, this must not break any features like tests/debug/profiling...
static const QStringList lockedOptions = {"buildtype", "debug", "backend", "optimization"};

inline Utils::TreeItem *makeBuildOptionTreeItem(CancellableOption *buildOption)
{
    return new BuildOptionTreeItem(buildOption);
}

BuidOptionsModel::BuidOptionsModel(QObject *parent)
    : Utils::TreeModel<>(parent)
{
    setHeader({Tr::tr("Key"), Tr::tr("Value")});
}

inline void groupPerSubprojectAndSection(
    const CancellableOptionsList &options,
    QMap<QString, QMap<QString, std::vector<CancellableOption *>>> &subprojectOptions,
    QMap<QString, std::vector<CancellableOption *>> &perSectionOptions)
{
    for (const std::unique_ptr<CancellableOption> &option : options) {
        if (option->subproject()) {
            subprojectOptions[*option->subproject()][option->section()].push_back(option.get());
        } else {
            perSectionOptions[option->section()].push_back(option.get());
        }
    }
}

void makeTree(Utils::TreeItem *root,
              const QMap<QString, std::vector<CancellableOption *>> &perSectioOptions)
{
    std::for_each(perSectioOptions.constKeyValueBegin(),
                  perSectioOptions.constKeyValueEnd(),
                  [root](const std::pair<QString, std::vector<CancellableOption *>> kv) {
                      const auto &options = kv.second;
                      auto sectionNode = new Utils::StaticTreeItem(kv.first);
                      for (CancellableOption *option : options) {
                          sectionNode->appendChild(makeBuildOptionTreeItem(option));
                      }
                      root->appendChild(sectionNode);
                  });
}

void BuidOptionsModel::setConfiguration(const BuildOptionsList &options)
{
    clear();
    m_options = decltype(m_options)();
    for (const BuildOptionsList::value_type &option : options) {
        m_options.emplace_back(
            std::make_unique<CancellableOption>(option.get(), lockedOptions.contains(option->name)));
    }
    {
        QMap<QString, QMap<QString, std::vector<CancellableOption *>>> subprojectOptions;
        QMap<QString, std::vector<CancellableOption *>> perSectionOptions;
        groupPerSubprojectAndSection(m_options, subprojectOptions, perSectionOptions);
        auto root = new Utils::TreeItem;
        makeTree(root, perSectionOptions);
        auto subProjects = new Utils::StaticTreeItem{"Subprojects"};
        std::for_each(subprojectOptions.constKeyValueBegin(),
                      subprojectOptions.constKeyValueEnd(),
                      [subProjects](
                          const std::pair<QString, QMap<QString, std::vector<CancellableOption *>>> kv) {
                          auto subProject = new Utils::StaticTreeItem{kv.first};
                          makeTree(subProject, kv.second);
                          subProjects->appendChild(subProject);
                      });
        root->appendChild(subProjects);
        setRootItem(root);
    }
}

bool BuidOptionsModel::setData(const QModelIndex &idx, const QVariant &data, int role)
{
    bool result = Utils::TreeModel<>::setData(idx, data, role);
    if (hasChanges())
        emit configurationChanged();
    return result;
}

QStringList BuidOptionsModel::changesAsMesonArgs()
{
    QStringList args;
    for (const std::unique_ptr<CancellableOption> &option : m_options) {
        if (option->hasChanged()) {
            args.push_back(option->mesonArg());
        }
    }
    return args;
}

bool BuidOptionsModel::hasChanges() const
{
    for (const std::unique_ptr<CancellableOption> &option : m_options) {
        if (option->hasChanged())
            return true;
    }
    return false;
}

QWidget *BuildOptionDelegate::makeWidget(QWidget *parent, const QVariant &data)
{
    auto type = data.userType();
    switch (type) {
    case QVariant::Int: {
        auto w = new QSpinBox{parent};
        w->setValue(data.toInt());
        return w;
    }
    case QVariant::Bool: {
        auto w = new QComboBox{parent};
        w->addItems({"false", "true"});
        w->setCurrentIndex(data.toBool());
        return w;
    }
    case QVariant::StringList: {
        auto w = new ArrayOptionLineEdit{parent};
        w->setPlainText(data.toStringList().join(" "));
        return w;
    }
    case QVariant::String: {
        auto w = new QLineEdit{parent};
        w->setText(data.toString());
        return w;
    }
    default: {
        if (type == qMetaTypeId<ComboData>()) {
            auto w = new QComboBox{parent};
            auto value = data.value<ComboData>();
            w->addItems(value.choices());
            w->setCurrentIndex(value.currentIndex());
            return w;
        }
        if (type == qMetaTypeId<FeatureData>()) {
            auto w = new QComboBox{parent};
            auto value = data.value<FeatureData>();
            w->addItems(value.choices());
            w->setCurrentIndex(value.currentIndex());
            return w;
        }
        return nullptr;
    }
    }
}

BuildOptionDelegate::BuildOptionDelegate(QObject *parent)
    : QStyledItemDelegate{parent}
{}

QWidget *BuildOptionDelegate::createEditor(QWidget *parent,
                                           const QStyleOptionViewItem &option,
                                           const QModelIndex &index) const
{
    auto data = index.data(Qt::EditRole);
    bool readOnly = index.data(Qt::UserRole).toBool();
    auto widget = makeWidget(parent, data);
    if (widget) {
        widget->setFocusPolicy(Qt::StrongFocus);
        widget->setDisabled(readOnly);
        return widget;
    }
    return QStyledItemDelegate::createEditor(parent, option, index);
}

void BuildOptionDelegate::setModelData(QWidget *editor,
                                       QAbstractItemModel *model,
                                       const QModelIndex &index) const
{
    ArrayOptionLineEdit *arrayWidget = qobject_cast<ArrayOptionLineEdit *>(editor);
    if (arrayWidget) {
        model->setData(index, QVariant::fromValue(arrayWidget->options()));
    } else {
        QStyledItemDelegate::setModelData(editor, model, index);
    }
}

} // namespace Internal
} // namespace MesonProjectManager
