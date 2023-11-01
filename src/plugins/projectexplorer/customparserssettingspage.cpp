// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "customparserssettingspage.h"

#include "customparser.h"
#include "customparserconfigdialog.h"
#include "projectexplorer.h"
#include "projectexplorerconstants.h"
#include "projectexplorertr.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QListWidget>
#include <QPushButton>
#include <QUuid>
#include <QVBoxLayout>

namespace ProjectExplorer {
namespace Internal {

class CustomParsersSettingsWidget final : public Core::IOptionsPageWidget
{
public:
    CustomParsersSettingsWidget()
    {
        m_customParsers = ProjectExplorerPlugin::customParsers();
        resetListView();

        const auto mainLayout = new QVBoxLayout(this);
        const auto widgetLayout = new QHBoxLayout;
        mainLayout->addLayout(widgetLayout);
        const auto hintLabel = new QLabel(Tr::tr(
            "Custom output parsers defined here can be enabled individually "
            "in the project's build or run settings."));
        mainLayout->addWidget(hintLabel);
        widgetLayout->addWidget(&m_parserListView);
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
            newParser.id = Utils::Id::fromString(QUuid::createUuid().toString());
            newParser.displayName = Tr::tr("New Parser");
            m_customParsers << newParser;
            resetListView();
        });
        connect(removeButton, &QPushButton::clicked, this, [this] {
            const QList<QListWidgetItem *> sel = m_parserListView.selectedItems();
            QTC_ASSERT(sel.size() == 1, return);
            m_customParsers.removeAt(m_parserListView.row(sel.first()));
            delete sel.first();
        });
        connect(editButton, &QPushButton::clicked, this, [this] {
            const QList<QListWidgetItem *> sel = m_parserListView.selectedItems();
            QTC_ASSERT(sel.size() == 1, return);
            CustomParserSettings &s = m_customParsers[m_parserListView.row(sel.first())];
            CustomParserConfigDialog dlg(this);
            dlg.setSettings(s);
            if (dlg.exec() != QDialog::Accepted)
                return;
            s.error = dlg.settings().error;
            s.warning = dlg.settings().warning;
        });

        connect(&m_parserListView, &QListWidget::itemChanged, this, [this](QListWidgetItem *item) {
            m_customParsers[m_parserListView.row(item)].displayName = item->text();
            resetListView();
        });

        const auto updateButtons = [this, removeButton, editButton] {
            const bool enable = !m_parserListView.selectedItems().isEmpty();
            removeButton->setEnabled(enable);
            editButton->setEnabled(enable);
        };
        updateButtons();
        connect(m_parserListView.selectionModel(), &QItemSelectionModel::selectionChanged,
                updateButtons);
    }

private:
    void apply() override { ProjectExplorerPlugin::setCustomParsers(m_customParsers); }

    void resetListView()
    {
        m_parserListView.clear();
        Utils::sort(m_customParsers,
                    [](const CustomParserSettings &s1, const CustomParserSettings &s2) {
            return s1.displayName < s2.displayName;
        });
        for (const CustomParserSettings &s : std::as_const(m_customParsers)) {
            const auto item = new QListWidgetItem(s.displayName);
            item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
            m_parserListView.addItem(item);
        }
    }

    QListWidget m_parserListView;
    QList<CustomParserSettings> m_customParsers;
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
