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

#include "commandmappings.h"
#include "commandsfile.h"

#include <coreplugin/dialogs/shortcutsettings.h>

#include <utils/headerviewstretcher.h>
#include <utils/fancylineedit.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

Q_DECLARE_METATYPE(Core::Internal::ShortcutItem*)

using namespace Utils;

namespace Core {
namespace Internal {

class CommandMappingsPrivate
{
public:
    CommandMappingsPrivate(CommandMappings *parent)
        : q(parent)
    {
        groupBox = new QGroupBox(parent);
        groupBox->setTitle(CommandMappings::tr("Command Mappings"));

        filterEdit = new FancyLineEdit(groupBox);
        filterEdit->setFiltering(true);

        commandList = new QTreeWidget(groupBox);
        commandList->setRootIsDecorated(false);
        commandList->setUniformRowHeights(true);
        commandList->setSortingEnabled(true);
        commandList->setColumnCount(3);

        QTreeWidgetItem *item = commandList->headerItem();
        item->setText(2, CommandMappings::tr("Target"));
        item->setText(1, CommandMappings::tr("Label"));
        item->setText(0, CommandMappings::tr("Command"));

        defaultButton = new QPushButton(CommandMappings::tr("Reset All"), groupBox);
        defaultButton->setToolTip(CommandMappings::tr("Reset all to default."));

        importButton = new QPushButton(CommandMappings::tr("Import..."), groupBox);
        exportButton = new QPushButton(CommandMappings::tr("Export..."), groupBox);

        QHBoxLayout *hboxLayout1 = new QHBoxLayout();
        hboxLayout1->addWidget(defaultButton);
        hboxLayout1->addStretch();
        hboxLayout1->addWidget(importButton);
        hboxLayout1->addWidget(exportButton);

        QHBoxLayout *hboxLayout = new QHBoxLayout();
        hboxLayout->addWidget(filterEdit);

        QVBoxLayout *vboxLayout1 = new QVBoxLayout(groupBox);
        vboxLayout1->addLayout(hboxLayout);
        vboxLayout1->addWidget(commandList);
        vboxLayout1->addLayout(hboxLayout1);

        QVBoxLayout *vboxLayout = new QVBoxLayout(parent);
        vboxLayout->addWidget(groupBox);

        q->connect(exportButton, &QPushButton::clicked,
                   q, &CommandMappings::exportAction);
        q->connect(importButton, &QPushButton::clicked,
                   q, &CommandMappings::importAction);
        q->connect(defaultButton, &QPushButton::clicked,
                   q, &CommandMappings::defaultAction);

        commandList->sortByColumn(0, Qt::AscendingOrder);

        q->connect(filterEdit, &FancyLineEdit::textChanged,
                   q, &CommandMappings::filterChanged);
        q->connect(commandList, &QTreeWidget::currentItemChanged,
                   q, &CommandMappings::currentCommandChanged);

        new HeaderViewStretcher(commandList->header(), 1);
    }

    CommandMappings *q;

    QGroupBox *groupBox;
    FancyLineEdit *filterEdit;
    QTreeWidget *commandList;
    QPushButton *defaultButton;
    QPushButton *importButton;
    QPushButton *exportButton;
};

} // namespace Internal

CommandMappings::CommandMappings(QWidget *parent)
    : QWidget(parent), d(new Internal::CommandMappingsPrivate(this))
{
}

CommandMappings::~CommandMappings()
{
   delete d;
}

void CommandMappings::setImportExportEnabled(bool enabled)
{
    d->importButton->setVisible(enabled);
    d->exportButton->setVisible(enabled);
}

QTreeWidget *CommandMappings::commandList() const
{
    return d->commandList;
}

void CommandMappings::setPageTitle(const QString &s)
{
    d->groupBox->setTitle(s);
}

void CommandMappings::setTargetHeader(const QString &s)
{
    d->commandList->setHeaderLabels({tr("Command"), tr("Label"), s});
}

void CommandMappings::filterChanged(const QString &f)
{
    for (int i = 0; i < d->commandList->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = d->commandList->topLevelItem(i);
        filter(f, item);
    }
}

bool CommandMappings::filter(const QString &filterString, QTreeWidgetItem *item)
{
    bool visible = filterString.isEmpty();
    int columnCount = item->columnCount();
    for (int i = 0; !visible && i < columnCount; ++i)
        visible |= !filterColumn(filterString, item, i);

    int childCount = item->childCount();
    if (childCount > 0) {
        // force visibility if this item matches
        QString leafFilterString = visible ? QString() : filterString;
        for (int i = 0; i < childCount; ++i) {
            QTreeWidgetItem *citem = item->child(i);
            visible |= !filter(leafFilterString, citem); // parent visible if any child visible
        }
    }
    item->setHidden(!visible);
    return !visible;
}

bool CommandMappings::filterColumn(const QString &filterString, QTreeWidgetItem *item,
                                   int column) const
{
    return !item->text(column).contains(filterString, Qt::CaseInsensitive);
}

void CommandMappings::setModified(QTreeWidgetItem *item , bool modified)
{
    QFont f = item->font(0);
    f.setItalic(modified);
    item->setFont(0, f);
    item->setFont(1, f);
    f.setBold(modified);
    item->setFont(2, f);
}

QString CommandMappings::filterText() const
{
    return d->filterEdit->text();
}

void CommandMappings::setFilterText(const QString &text)
{
    d->filterEdit->setText(text);
}

} // namespace Core
