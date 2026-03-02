// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "commandmappings.h"

#include "../coreplugintr.h"

#include <utils/fancylineedit.h>
#include <utils/headerviewstretcher.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QHeaderView>
#include <QGroupBox>
#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QTreeWidgetItem>

using namespace Utils;

namespace Core {
namespace Internal {

class CommandMappingsPrivate : public QWidget
{
public:
    CommandMappingsPrivate(CommandMappings *parent)
        : q(parent)
    {
        filterEdit.setFiltering(true);

        commandList.setRootIsDecorated(false);
        commandList.setUniformRowHeights(true);
        commandList.setSortingEnabled(true);
        commandList.setColumnCount(3);

        QTreeWidgetItem *item = commandList.headerItem();
        item->setText(2, ::Core::Tr::tr("Target"));
        item->setText(1, ::Core::Tr::tr("Label"));
        item->setText(0, ::Core::Tr::tr("Command"));

        defaultButton.setText(::Core::Tr::tr("Reset All"));
        defaultButton.setToolTip(::Core::Tr::tr("Reset all to default."));

        resetButton.setText(::Core::Tr::tr("Reset"));
        resetButton.setToolTip(::Core::Tr::tr("Reset to default."));
        resetButton.setVisible(false);

        importButton.setText(::Core::Tr::tr("Import..."));
        exportButton.setText(::Core::Tr::tr("Export..."));

        using namespace Layouting;
        Column {
            Group {
                title(::Core::Tr::tr("Command Mappings")),
                bindTo(&groupBox),
                Column {
                    filterEdit,
                    commandList,
                    Row { defaultButton, resetButton, st, importButton, exportButton },
                },
            },
        }.attachTo(this);

        connect(&exportButton, &QPushButton::clicked,
                q, &CommandMappings::exportRequested);
        connect(&importButton, &QPushButton::clicked,
                q, &CommandMappings::importRequested);
        connect(&defaultButton, &QPushButton::clicked,
                q, &CommandMappings::defaultRequested);
        connect(&resetButton, &QPushButton::clicked, q, &CommandMappings::resetRequested);

        commandList.sortByColumn(0, Qt::AscendingOrder);

        connect(&filterEdit, &FancyLineEdit::textChanged,
                q, &CommandMappings::filterChanged);
        connect(&commandList, &QTreeWidget::currentItemChanged,
                q, &CommandMappings::currentCommandChanged);

        new HeaderViewStretcher(commandList.header(), 2);

        m_columnFilter = [](const QString &filterString, QTreeWidgetItem *item, int column) {
                return !item->text(column).contains(filterString, Qt::CaseInsensitive);
        };
    }

    bool filterColumn(const QString &filterString, QTreeWidgetItem *item, int column) const
    {
        QTC_ASSERT(m_columnFilter, return false);
        return m_columnFilter(filterString, item, column);
    }

    CommandMappings *q;

    QGroupBox *groupBox;
    FancyLineEdit filterEdit;
    QTreeWidget commandList;
    QPushButton defaultButton;
    QPushButton resetButton;
    QPushButton importButton;
    QPushButton exportButton;

    CommandMappings::ColumnFilter m_columnFilter;
};

} // namespace Internal

/*!
    \class Core::CommandMappings
    \inmodule QtCreator
    \internal
*/

CommandMappings::CommandMappings(QWidget *parent)
    : QObject(parent), d(new Internal::CommandMappingsPrivate(this))
{
}

CommandMappings::~CommandMappings()
{
   delete d;
}

QWidget *CommandMappings::widget() const
{
    return d;
}

void CommandMappings::setImportExportEnabled(bool enabled)
{
    d->importButton.setVisible(enabled);
    d->exportButton.setVisible(enabled);
}

void CommandMappings::setResetVisible(bool visible)
{
    d->resetButton.setVisible(visible);
}

void CommandMappings::setColumnFilter(const ColumnFilter &filter)
{
    d->m_columnFilter = filter;
}

QTreeWidget *CommandMappings::commandList() const
{
    return &d->commandList;
}

void CommandMappings::setPageTitle(const QString &s)
{
    d->groupBox->setTitle(s);
}

void CommandMappings::setTargetHeader(const QString &s)
{
    d->commandList.setHeaderLabels({::Core::Tr::tr("Command"), ::Core::Tr::tr("Label"), s});
}

void CommandMappings::filterChanged(const QString &f)
{
    for (int i = 0; i < d->commandList.topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = d->commandList.topLevelItem(i);
        filter(f, item);
    }
}

bool CommandMappings::filter(const QString &filterString, QTreeWidgetItem *item)
{
    bool visible = filterString.isEmpty();
    int columnCount = item->columnCount();
    for (int i = 0; !visible && i < columnCount; ++i)
        visible |= !d->filterColumn(filterString, item, i);

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

void CommandMappings::setModified(QTreeWidgetItem *item, bool modified)
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
    return d->filterEdit.text();
}

void CommandMappings::setFilterText(const QString &text)
{
    d->filterEdit.setText(text);
}

} // namespace Core
