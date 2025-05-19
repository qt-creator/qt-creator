// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "customparserssettingspage.h"

#include "customparser.h"
#include "customparserconfigdialog.h"
#include "projectexplorer.h"
#include "projectexplorerconstants.h"
#include "projectexplorertr.h"

#include <utils/algorithm.h>
#include <utils/itemviews.h>
#include <utils/qtcassert.h>

#include <QAbstractTableModel>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QPushButton>
#include <QVBoxLayout>

namespace ProjectExplorer {
namespace Internal {

class CustomParsersModel : public QAbstractTableModel
{
public:
    explicit CustomParsersModel(QObject *parent = nullptr);

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    bool add(const ProjectExplorer::CustomParserSettings& s);
    bool remove(const QModelIndex &index);

    void apply();

protected:
    QList<ProjectExplorer::CustomParserSettings> m_customParsers;
};

CustomParsersModel::CustomParsersModel(QObject *parent)
    : QAbstractTableModel(parent)
    , m_customParsers(ProjectExplorerPlugin::customParsers())
{
    connect(
        ProjectExplorerPlugin::instance(),
        &ProjectExplorerPlugin::customParsersChanged,
        this,
        [this] {
            beginResetModel();
            m_customParsers = ProjectExplorerPlugin::customParsers();
            endResetModel();
        });
}

QVariant CustomParsersModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    QVariant result;
    if (orientation == Qt::Vertical)
        return QVariant();

    switch (role) {
    case Qt::DisplayRole:
        switch (section) {
        case 0:
            result = Tr::tr("Name");
            break;
        case 1:
            result = Tr::tr("Build default");
            break;
        case 2:
            result = Tr::tr("Run default");
            break;
        }
        break;
    case Qt::ToolTipRole:
        switch (section) {
        case 0:
            result = Tr::tr("The name of the custom parser.");
            break;
        case 1:
            result = Tr::tr("This custom parser is used by default for all build configurations of "
                            "the project.");
            break;
        case 2:
            result = Tr::tr(
                "This custom parser is used by default for all run configurations of the project.");
            break;
        }
    }
    return result;
}

int CustomParsersModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 3;
}

int CustomParsersModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return m_customParsers.size();
}

QVariant CustomParsersModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const CustomParserSettings &s = m_customParsers[index.row()];

    switch (role) {
    case Qt::CheckStateRole:
        switch (index.column()) {
        case 1:
            return s.buildDefault ? Qt::Checked : Qt::Unchecked;
        case 2:
            return s.runDefault ? Qt::Checked : Qt::Unchecked;
        }
        break;

    case Qt::DisplayRole:
        switch (index.column()) {
        case 0:
            return s.displayName;
        }
        break;

    case Qt::EditRole:
        switch (index.column()) {
        case 1:
            return s.buildDefault;
        case 2:
            return s.runDefault;
        }
        break;

    case Qt::TextAlignmentRole:
        switch (index.column()) {
        case 1:
        case 2:
            return Qt::AlignCenter;
        }
        break;

    case Qt::UserRole:
        return QVariant::fromValue<CustomParserSettings>(s);
    }

    return QVariant();
}

bool CustomParsersModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;

    if (index.row() >= m_customParsers.size())
        return false;

    CustomParserSettings &s = m_customParsers[index.row()];

    bool result = true;
    switch (role) {
    case Qt::EditRole:
        switch (index.column()) {
        case 0:
            s.displayName = value.toString();
            emit dataChanged(index, index);
            break;
        }
        break;

    case Qt::CheckStateRole:
        switch (index.column()) {
        case 1:
            s.buildDefault = value == Qt::Checked;
            emit dataChanged(index, index);
            break;
        case 2:
            s.runDefault = value == Qt::Checked;
            emit dataChanged(index, index);
            break;
        }
        break;

    case Qt::UserRole:
        if (value.canConvert<CustomParserSettings>()) {
            s = value.value<CustomParserSettings>();
            emit dataChanged(index, index);
        } else
            result = false;
    }
    return result;
}

Qt::ItemFlags CustomParsersModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    if (index.column() > 0)
        flags |= Qt::ItemIsUserCheckable;
    else
        flags |= Qt::ItemIsEditable;

    return flags;
}

bool CustomParsersModel::add(const CustomParserSettings &s)
{
    beginInsertRows(index(-1, -1), m_customParsers.size(), m_customParsers.size());
    m_customParsers.append(s);
    endInsertRows();
    return true;
}

bool CustomParsersModel::remove(const QModelIndex &index)
{
    beginRemoveRows(index.parent(), index.row(), index.row());
    m_customParsers.removeAt(index.row());
    endRemoveRows();
    return true;
}

void CustomParsersModel::apply()
{
    ProjectExplorerPlugin::setCustomParsers(m_customParsers);
}

class CustomParsersSettingsWidget final : public Core::IOptionsPageWidget
{
public:
    CustomParsersSettingsWidget()
    {
        Utils::TreeView *parserView = new Utils::TreeView(this);
        parserView->setModel(&m_model);
        parserView->setSelectionMode(QAbstractItemView::SingleSelection);
        parserView->setSelectionBehavior(QAbstractItemView::SelectRows);

        const auto mainLayout = new QVBoxLayout(this);
        const auto widgetLayout = new QHBoxLayout;
        mainLayout->addLayout(widgetLayout);
        const auto hintLabel = new QLabel(Tr::tr(
            "Custom output parsers defined here can be enabled individually "
            "in the project's build or run settings."));
        mainLayout->addWidget(hintLabel);
        widgetLayout->addWidget(parserView);
        const auto buttonLayout = new QVBoxLayout;
        widgetLayout->addLayout(buttonLayout);
        const auto addButton = new QPushButton(Tr::tr("Add..."));
        const auto removeButton = new QPushButton(Tr::tr("Remove"));
        const auto editButton = new QPushButton("Edit...");
        buttonLayout->addWidget(addButton);
        buttonLayout->addWidget(removeButton);
        buttonLayout->addWidget(editButton);
        buttonLayout->addStretch(1);

        connect(addButton, &QPushButton::clicked, this, [this] {
            CustomParserConfigDialog dlg(this);
            dlg.setSettings(CustomParserSettings());
            if (dlg.exec() != QDialog::Accepted)
                return;
            CustomParserSettings newParser = dlg.settings();
            newParser.id = Utils::Id::generate();
            newParser.displayName = Tr::tr("New Parser");
            m_model.add(newParser);
        });

        connect(removeButton, &QPushButton::clicked, this, [this, parserView] {
            m_model.remove(parserView->currentIndex());
        });

        connect(editButton, &QPushButton::clicked, this, [this, parserView] {
            CustomParserSettings s = m_model.data(parserView->currentIndex(), Qt::UserRole).value<CustomParserSettings>();
            CustomParserConfigDialog dlg(this);
            dlg.setSettings(s);
            if (dlg.exec() != QDialog::Accepted)
                return;
            s.error = dlg.settings().error;
            s.warning = dlg.settings().warning;
            m_model.setData(parserView->currentIndex(), QVariant::fromValue<CustomParserSettings>(s), Qt::UserRole);
        });

        const auto updateButtons = [editButton, parserView, removeButton] {
            const bool enable = parserView->currentIndex().isValid();
            removeButton->setEnabled(enable);
            editButton->setEnabled(enable);
        };
        updateButtons();
        connect(parserView->selectionModel(), &QItemSelectionModel::selectionChanged,
            updateButtons);
    }

private:
    void apply() override { m_model.apply(); }

    CustomParsersModel m_model;
};

CustomParsersSettingsPage::CustomParsersSettingsPage()
{
    setId(Constants::CUSTOM_PARSERS_SETTINGS_PAGE_ID);
    setDisplayName(Tr::tr("Custom Output Parsers"));
    setCategory(Constants::BUILD_AND_RUN_SETTINGS_CATEGORY);
    setWidgetCreator([] { return new CustomParsersSettingsWidget; });
}

} // namespace Internal
} // namespace ProjectExplorer
