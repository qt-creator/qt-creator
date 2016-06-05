/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "todoprojectsettingswidget.h"
#include "ui_todoprojectsettingswidget.h"
#include "constants.h"

#include <projectexplorer/project.h>

static QString excludePlaceholder()
{
    return Todo::Internal::TodoProjectSettingsWidget::tr("<Enter regular expression to exclude>");
}

namespace Todo {
namespace Internal {

TodoProjectSettingsWidget::TodoProjectSettingsWidget(ProjectExplorer::Project *project) :
    ui(new Ui::TodoProjectSettingsWidget),
    m_project(project)
{
    ui->setupUi(this);

    setExcludedPatternsButtonsEnabled();
    connect(ui->addExcludedPatternButton, &QPushButton::clicked,
            this, &TodoProjectSettingsWidget::addExcludedPatternButtonClicked);
    connect(ui->removeExcludedPatternButton, &QPushButton::clicked,
            this, &TodoProjectSettingsWidget::removeExcludedPatternButtonClicked);
    connect(ui->excludedPatternsList, &QListWidget::itemChanged,
            this, &TodoProjectSettingsWidget::excludedPatternChanged, Qt::QueuedConnection);
    connect(ui->excludedPatternsList, &QListWidget::itemSelectionChanged,
            this, &TodoProjectSettingsWidget::setExcludedPatternsButtonsEnabled);

    loadSettings();
}

TodoProjectSettingsWidget::~TodoProjectSettingsWidget()
{
    delete ui;
}

QListWidgetItem *TodoProjectSettingsWidget::addToExcludedPatternsList(const QString &pattern)
{
    QListWidgetItem *item = new QListWidgetItem(pattern);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    prepareItem(item);
    ui->excludedPatternsList->addItem(item);
    return item;
}

void TodoProjectSettingsWidget::loadSettings()
{
    QVariant s = m_project->namedSettings(QLatin1String(Constants::SETTINGS_NAME_KEY));
    QVariantMap settings = s.toMap();
    ui->excludedPatternsList->clear();
    for (const QVariant &pattern : settings[QLatin1String(Constants::EXCLUDES_LIST_KEY)].toList())
        addToExcludedPatternsList(pattern.toString());
}

void TodoProjectSettingsWidget::saveSettings()
{
    QVariantMap settings;
    QVariantList excludes;

    for (int i = 0; i < ui->excludedPatternsList->count(); ++i)
        excludes << ui->excludedPatternsList->item(i)->text();

    settings[QLatin1String(Constants::EXCLUDES_LIST_KEY)] = excludes;

    m_project->setNamedSettings(QLatin1String(Constants::SETTINGS_NAME_KEY), settings);
    emit projectSettingsChanged();
}

void TodoProjectSettingsWidget::prepareItem(QListWidgetItem *item) const
{
    if (QRegExp(item->text()).isValid())
        item->setForeground(QBrush(ui->excludedPatternsList->palette().color(QPalette::Active, QPalette::Text)));
    else
        item->setForeground(QBrush(Qt::red));
}

void TodoProjectSettingsWidget::addExcludedPatternButtonClicked()
{
    if (ui->excludedPatternsList->findItems(excludePlaceholder(), Qt::MatchFixedString).count())
        return;
    ui->excludedPatternsList->editItem(addToExcludedPatternsList(excludePlaceholder()));
}

void TodoProjectSettingsWidget::removeExcludedPatternButtonClicked()
{
    delete ui->excludedPatternsList->takeItem(ui->excludedPatternsList->currentRow());
    saveSettings();
}

void TodoProjectSettingsWidget::setExcludedPatternsButtonsEnabled()
{
    bool isSomethingSelected = ui->excludedPatternsList->selectedItems().count() != 0;
    ui->removeExcludedPatternButton->setEnabled(isSomethingSelected);
}

void TodoProjectSettingsWidget::excludedPatternChanged(QListWidgetItem *item)
{
    if (item->text().isEmpty() || item->text() == excludePlaceholder()) {
        ui->excludedPatternsList->removeItemWidget(item);
        delete item;
    } else {
        prepareItem(item);
    }
    saveSettings();
    ui->excludedPatternsList->setCurrentItem(nullptr);
}

} // namespace Internal
} // namespace Todo
