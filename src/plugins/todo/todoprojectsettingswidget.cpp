// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "todoprojectsettingswidget.h"

#include "constants.h"
#include "todotr.h"

#include <projectexplorer/project.h>

#include <utils/layoutbuilder.h>

#include <QListWidget>
#include <QPushButton>

namespace Todo {
namespace Internal {

static QString excludePlaceholder()
{
    return Tr::tr("<Enter regular expression to exclude>");
}

TodoProjectSettingsWidget::TodoProjectSettingsWidget(ProjectExplorer::Project *project)
    : m_project(project)
{
    m_excludedPatternsList = new QListWidget;
    m_excludedPatternsList->setSortingEnabled(true);
    m_excludedPatternsList->setToolTip(Tr::tr("Regular expressions for file paths to be excluded from scanning."));

    m_removeExcludedPatternButton = new QPushButton(Tr::tr("Remove"));

    auto addExcludedPatternButton = new QPushButton(Tr::tr("Add"));

    using namespace Layouting;

    Column {
        Group {
            title(Tr::tr("Excluded Files")),
            Row {
                m_excludedPatternsList,
                Column {
                    addExcludedPatternButton,
                    m_removeExcludedPatternButton,
                    st
                }
            }
        }
    }.attachTo(this);

    setExcludedPatternsButtonsEnabled();
    setGlobalSettingsId(Constants::TODO_SETTINGS);
    connect(addExcludedPatternButton,
            &QPushButton::clicked,
            this,
            &TodoProjectSettingsWidget::addExcludedPatternButtonClicked);
    connect(m_removeExcludedPatternButton, &QPushButton::clicked,
            this, &TodoProjectSettingsWidget::removeExcludedPatternButtonClicked);
    connect(m_excludedPatternsList, &QListWidget::itemChanged,
            this, &TodoProjectSettingsWidget::excludedPatternChanged, Qt::QueuedConnection);
    connect(m_excludedPatternsList, &QListWidget::itemSelectionChanged,
            this, &TodoProjectSettingsWidget::setExcludedPatternsButtonsEnabled);

    loadSettings();
}

TodoProjectSettingsWidget::~TodoProjectSettingsWidget() = default;

QListWidgetItem *TodoProjectSettingsWidget::addToExcludedPatternsList(const QString &pattern)
{
    auto item = new QListWidgetItem(pattern);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    prepareItem(item);
    m_excludedPatternsList->addItem(item);
    return item;
}

void TodoProjectSettingsWidget::loadSettings()
{
    QVariant s = m_project->namedSettings(Constants::SETTINGS_NAME_KEY);
    QVariantMap settings = s.toMap();
    m_excludedPatternsList->clear();
    for (const QVariant &pattern : settings[Constants::EXCLUDES_LIST_KEY].toList())
        addToExcludedPatternsList(pattern.toString());
}

void TodoProjectSettingsWidget::saveSettings()
{
    QVariantMap settings;
    QVariantList excludes;

    for (int i = 0; i < m_excludedPatternsList->count(); ++i)
        excludes << m_excludedPatternsList->item(i)->text();

    settings[Constants::EXCLUDES_LIST_KEY] = excludes;

    m_project->setNamedSettings(Constants::SETTINGS_NAME_KEY, settings);
    emit projectSettingsChanged();
}

void TodoProjectSettingsWidget::prepareItem(QListWidgetItem *item) const
{
    if (QRegularExpression(item->text()).isValid())
        item->setForeground(QBrush(m_excludedPatternsList->palette().color(QPalette::Active, QPalette::Text)));
    else
        item->setForeground(QBrush(Qt::red));
}

void TodoProjectSettingsWidget::addExcludedPatternButtonClicked()
{
    if (!m_excludedPatternsList->findItems(excludePlaceholder(), Qt::MatchFixedString).isEmpty())
        return;
    m_excludedPatternsList->editItem(addToExcludedPatternsList(excludePlaceholder()));
}

void TodoProjectSettingsWidget::removeExcludedPatternButtonClicked()
{
    delete m_excludedPatternsList->takeItem(m_excludedPatternsList->currentRow());
    saveSettings();
}

void TodoProjectSettingsWidget::setExcludedPatternsButtonsEnabled()
{
    const bool isSomethingSelected = !m_excludedPatternsList->selectedItems().isEmpty();
    m_removeExcludedPatternButton->setEnabled(isSomethingSelected);
}

void TodoProjectSettingsWidget::excludedPatternChanged(QListWidgetItem *item)
{
    if (item->text().isEmpty() || item->text() == excludePlaceholder()) {
        m_excludedPatternsList->removeItemWidget(item);
        delete item;
    } else {
        prepareItem(item);
    }
    saveSettings();
    m_excludedPatternsList->setCurrentItem(nullptr);
}

} // namespace Internal
} // namespace Todo
