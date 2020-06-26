/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "customparserssettingspage.h"

#include "customparser.h"
#include "customparserconfigdialog.h"
#include "projectexplorer.h"
#include "projectexplorerconstants.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QHBoxLayout>
#include <QList>
#include <QListWidget>
#include <QPushButton>
#include <QUuid>
#include <QVBoxLayout>

namespace ProjectExplorer {
namespace Internal {

class CustomParsersSettingsWidget final : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(ProjectExplorer::Internal::CustomParsersSettingsPage)

public:
    CustomParsersSettingsWidget()
    {
        m_customParsers = ProjectExplorerPlugin::customParsers();
        resetListView();

        const auto mainLayout = new QVBoxLayout(this);
        const auto widgetLayout = new QHBoxLayout;
        mainLayout->addLayout(widgetLayout);
        widgetLayout->addWidget(&m_parserListView);
        const auto buttonLayout = new QVBoxLayout;
        widgetLayout->addLayout(buttonLayout);
        const auto addButton = new QPushButton(tr("Add..."));
        const auto removeButton = new QPushButton(tr("Remove"));
        const auto editButton = new QPushButton("Edit...");
        buttonLayout->addWidget(addButton);
        buttonLayout->addWidget(removeButton);
        buttonLayout->addWidget(editButton);
        buttonLayout->addStretch(1);

        connect(addButton, &QPushButton::clicked, [this] {
            CustomParserConfigDialog dlg(this);
            dlg.setSettings(CustomParserSettings());
            if (dlg.exec() != QDialog::Accepted)
                return;
            CustomParserSettings newParser = dlg.settings();
            newParser.id = Utils::Id::fromString(QUuid::createUuid().toString());
            newParser.displayName = tr("New Parser");
            m_customParsers << newParser;
            resetListView();
        });
        connect(removeButton, &QPushButton::clicked, [this] {
            const QList<QListWidgetItem *> sel = m_parserListView.selectedItems();
            QTC_ASSERT(sel.size() == 1, return);
            m_customParsers.removeAt(m_parserListView.row(sel.first()));
            delete sel.first();
        });
        connect(editButton, &QPushButton::clicked, [this] {
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

        connect(&m_parserListView, &QListWidget::itemChanged, [this](QListWidgetItem *item) {
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
        for (const CustomParserSettings &s : qAsConst(m_customParsers)) {
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
    setDisplayName(CustomParsersSettingsWidget::tr("Custom Output Parsers"));
    setCategory(Constants::BUILD_AND_RUN_SETTINGS_CATEGORY);
    setWidgetCreator([] { return new CustomParsersSettingsWidget; });
}

} // namespace Internal
} // namespace ProjectExplorer
