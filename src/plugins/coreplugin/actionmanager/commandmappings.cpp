/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "commandmappings.h"
#include "shortcutsettings.h"
#include "ui_commandmappings.h"
#include "actionmanager_p.h"
#include "actionmanager/command.h"
#include "command_p.h"
#include "commandsfile.h"
#include "coreconstants.h"
#include "documentmanager.h"
#include "icore.h"
#include "id.h"

#include <utils/treewidgetcolumnstretcher.h>

#include <QKeyEvent>
#include <QShortcut>
#include <QHeaderView>
#include <QTreeWidgetItem>
#include <QFileDialog>
#include <QCoreApplication>
#include <QtDebug>

Q_DECLARE_METATYPE(Core::Internal::ShortcutItem*)

using namespace Core;
using namespace Core::Internal;

CommandMappings::CommandMappings(QObject *parent)
    : IOptionsPage(parent), m_page(0)
{
}

// IOptionsPage

QWidget *CommandMappings::createPage(QWidget *parent)
{
    m_page = new Ui::CommandMappings();
    QWidget *w = new QWidget(parent);
    m_page->setupUi(w);
    m_page->targetEdit->setAutoHideButton(Utils::FancyLineEdit::Right, true);
    m_page->targetEdit->installEventFilter(this);

    connect(m_page->targetEdit, SIGNAL(buttonClicked(Utils::FancyLineEdit::Side)),
        this, SLOT(removeTargetIdentifier()));
    connect(m_page->resetButton, SIGNAL(clicked()),
        this, SLOT(resetTargetIdentifier()));
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
    connect(m_page->commandList, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
        this, SLOT(commandChanged(QTreeWidgetItem*)));
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
    if (!m_page) // page was never shown
        return;
    delete m_page;
    m_page = 0;
}

void CommandMappings::commandChanged(QTreeWidgetItem *current)
{
    if (!current || !current->data(0, Qt::UserRole).isValid()) {
        m_page->targetEdit->setText(QString());
        m_page->targetEditGroup->setEnabled(false);
        return;
    }
    m_page->targetEditGroup->setEnabled(true);
}

void CommandMappings::filterChanged(const QString &f)
{
    if (!m_page)
        return;
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
    if (!m_page)
        return QString();
    return m_page->filterEdit->text();
}
