/****************************************************************************
**
** Copyright (C) 2016 Nicolas Arnaud-Cormos
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

#include "macrooptionswidget.h"
#include "ui_macrooptionswidget.h"
#include "macrosconstants.h"
#include "macromanager.h"
#include "macro.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/id.h>

#include <QDir>
#include <QFileInfo>
#include <QTreeWidget>
#include <QTreeWidgetItem>

namespace {
    int NAME_ROLE = Qt::UserRole;
    int WRITE_ROLE = Qt::UserRole+1;
}

using namespace Macros;
using namespace Macros::Internal;


MacroOptionsWidget::MacroOptionsWidget(QWidget *parent) :
    QWidget(parent),
    m_ui(new Ui::MacroOptionsWidget),
    m_changingCurrent(false)
{
    m_ui->setupUi(this);

    connect(m_ui->treeWidget, &QTreeWidget::currentItemChanged,
            this, &MacroOptionsWidget::changeCurrentItem);
    connect(m_ui->removeButton, &QPushButton::clicked,
            this, &MacroOptionsWidget::remove);
    connect(m_ui->description, &QLineEdit::textChanged,
            this, &MacroOptionsWidget::changeDescription);

    m_ui->treeWidget->header()->setSortIndicator(0, Qt::AscendingOrder);

    initialize();
}

MacroOptionsWidget::~MacroOptionsWidget()
{
    delete m_ui;
}

void MacroOptionsWidget::initialize()
{
    m_macroToRemove.clear();
    m_macroToChange.clear();
    m_ui->treeWidget->clear();

    // Create the treeview
    createTable();
}

void MacroOptionsWidget::createTable()
{
    QDir dir(MacroManager::macrosDirectory());
    const Core::Id base = Core::Id(Constants::PREFIX_MACRO);
    QMapIterator<QString, Macro *> it(MacroManager::macros());
    while (it.hasNext()) {
        it.next();
        Macro *macro = it.value();
        QFileInfo fileInfo(macro->fileName());
        if (fileInfo.absoluteDir() == dir.absolutePath()) {
            QTreeWidgetItem *macroItem = new QTreeWidgetItem(m_ui->treeWidget);
            macroItem->setText(0, macro->displayName());
            macroItem->setText(1, macro->description());
            macroItem->setData(0, NAME_ROLE, macro->displayName());
            macroItem->setData(0, WRITE_ROLE, macro->isWritable());

            Core::Command *command =
                    Core::ActionManager::command(base.withSuffix(macro->displayName()));
            if (command && command->action())
                macroItem->setText(2, command->action()->shortcut().toString());
        }
    }
}

void MacroOptionsWidget::changeCurrentItem(QTreeWidgetItem *current)
{
    m_changingCurrent = true;
    if (!current) {
        m_ui->removeButton->setEnabled(false);
        m_ui->description->clear();
        m_ui->macroGroup->setEnabled(false);
    } else {
        m_ui->removeButton->setEnabled(true);
        m_ui->description->setText(current->text(1));
        m_ui->description->setEnabled(current->data(0, WRITE_ROLE).toBool());
        m_ui->macroGroup->setEnabled(true);
    }
    m_changingCurrent = false;
}

void MacroOptionsWidget::remove()
{
    QTreeWidgetItem *current = m_ui->treeWidget->currentItem();
    m_macroToRemove.append(current->data(0, NAME_ROLE).toString());
    delete current;
}

void MacroOptionsWidget::apply()
{
    // Remove macro
    foreach (const QString &name, m_macroToRemove) {
        MacroManager::instance()->deleteMacro(name);
        m_macroToChange.remove(name);
    }

    // Change macro
    QMapIterator<QString, QString> it(m_macroToChange);
    while (it.hasNext()) {
        it.next();
        MacroManager::instance()->changeMacro(it.key(), it.value());
    }

    // Reinitialize the page
    initialize();
}

void MacroOptionsWidget::changeDescription(const QString &description)
{
    QTreeWidgetItem *current = m_ui->treeWidget->currentItem();
    if (m_changingCurrent || !current)
        return;

    QString macroName = current->data(0, NAME_ROLE).toString();
    m_macroToChange[macroName] = description;
    current->setText(1, description);
    QFont font = current->font(1);
    font.setItalic(true);
    current->setFont(1, font);
}
