/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "commandmappings.h"
#include "ui_commandmappings.h"
#include "actionmanager_p.h"
#include "actionmanager/command.h"
#include "command_p.h"
#include "commandsfile.h"
#include "coreconstants.h"
#include "filemanager.h"
#include "icore.h"
#include "uniqueidmanager.h"

#include <utils/treewidgetcolumnstretcher.h>

#include <QtGui/QKeyEvent>
#include <QtGui/QShortcut>
#include <QtGui/QHeaderView>
#include <QtGui/QTreeWidgetItem>
#include <QtGui/QFileDialog>
#include <QtCore/QCoreApplication>
#include <QtDebug>

Q_DECLARE_METATYPE(Core::Internal::ShortcutItem*);

using namespace Core;
using namespace Core::Internal;

CommandMappings::CommandMappings(QObject *parent)
    : IOptionsPage(parent)
{
}

CommandMappings::~CommandMappings()
{
}

// IOptionsPage

QWidget *CommandMappings::createPage(QWidget *parent)
{
    m_page = new Ui_CommandMappings();
    QWidget *w = new QWidget(parent);
    m_page->setupUi(w);
    m_page->targetEdit->setPixmap(QPixmap(Constants::ICON_RESET));
    m_page->targetEdit->setSide(Utils::FancyLineEdit::Right);
    m_page->targetEdit->installEventFilter(this);

    connect(m_page->targetEdit, SIGNAL(buttonClicked()),
        this, SLOT(resetTargetIdentifier()));
    connect(m_page->removeButton, SIGNAL(clicked()),
        this, SLOT(removeTargetIdentifier()));
    connect(m_page->exportButton, SIGNAL(clicked()),
        this, SLOT(exportAction()));
    connect(m_page->importButton, SIGNAL(clicked()),
        this, SLOT(importAction()));
    connect(m_page->defaultButton, SIGNAL(clicked()),
        this, SLOT(defaultAction()));

    initialize();

    m_page->commandList->sortByColumn(0, Qt::AscendingOrder);

    connect(m_page->filterEdit, SIGNAL(textChanged(QString)),
        this, SLOT(filterChanged(QString)));
    connect(m_page->commandList, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
        this, SLOT(commandChanged(QTreeWidgetItem *)));
    connect(m_page->targetEdit, SIGNAL(textChanged(QString)),
        this, SLOT(targetIdentifierChanged()));

    new Utils::TreeWidgetColumnStretcher(m_page->commandList, 1);

    commandChanged(0);

    return w;
}

void CommandMappings::setImportExportEnabled(bool enabled)
{
    m_page->importButton->setVisible(enabled);
    m_page->exportButton->setVisible(enabled);
}

QTreeWidget *CommandMappings::commandList() const
{
    return m_page->commandList;
}

QLineEdit *CommandMappings::targetEdit() const
{
    return m_page->targetEdit;
}

void CommandMappings::setPageTitle(const QString &s)
{
    m_page->groupBox->setTitle(s);
}

void CommandMappings::setTargetLabelText(const QString &s)
{
    m_page->targetEditLabel->setText(s);
}

void CommandMappings::setTargetEditTitle(const QString &s)
{
    m_page->targetEditGroup->setTitle(s);
}

void CommandMappings::setTargetHeader(const QString &s)
{
    m_page->commandList->setHeaderLabels(QStringList() << tr("Command") << tr("Label") << s);
}

void CommandMappings::finish()
{
    delete m_page;
}

void CommandMappings::commandChanged(QTreeWidgetItem *current)
{
    if (!current || !current->data(0, Qt::UserRole).isValid()) {
        m_page->targetEdit->setText("");
        m_page->targetEditGroup->setEnabled(false);
        return;
    }
    m_page->targetEditGroup->setEnabled(true);
}

void CommandMappings::filterChanged(const QString &f)
{
    for (int i=0; i<m_page->commandList->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = m_page->commandList->topLevelItem(i);
        item->setHidden(filter(f, item));
    }
}

bool CommandMappings::filter(const QString &f, const QTreeWidgetItem *item)
{

    if (QTreeWidgetItem *parent = item->parent()) {
        if (parent->text(0).contains(f, Qt::CaseInsensitive))
            return false;
    }

    if (item->childCount() == 0) {
        if (f.isEmpty())
            return false;
        for (int i = 0; i < item->columnCount(); ++i) {
            if (item->text(i).contains(f, Qt::CaseInsensitive))
                return false;
        }
        return true;
    }

    bool found = false;
    for (int i = 0; i < item->childCount(); ++i) {
        QTreeWidgetItem *citem = item->child(i);
        if (filter(f, citem)) {
            citem->setHidden(true);
        } else {
            citem->setHidden(false);
            found = true;
        }
    }
    return !found;
}
