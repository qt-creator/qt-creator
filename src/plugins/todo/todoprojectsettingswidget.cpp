/**************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#include "todoprojectsettingswidget.h"
#include "ui_todoprojectsettingswidget.h"
#include "constants.h"

#include <projectexplorer/project.h>

namespace Todo {
namespace Internal {

static const QString EXCLUDE_PLACEHOLDER = QObject::tr("<Enter regular expression to exclude>");

TodoProjectSettingsWidget::TodoProjectSettingsWidget(ProjectExplorer::Project *project) :
    QWidget(0),
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
    if (ui->excludedPatternsList->findItems(EXCLUDE_PLACEHOLDER, Qt::MatchFixedString).count())
        return;
    ui->excludedPatternsList->editItem(addToExcludedPatternsList(EXCLUDE_PLACEHOLDER));
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
    if (item->text().isEmpty() || item->text() == EXCLUDE_PLACEHOLDER) {
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
