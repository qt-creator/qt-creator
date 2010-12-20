/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nicolas Arnaud-Cormos.
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "macrooptionswidget.h"
#include "ui_macrooptionswidget.h"
#include "macrosettings.h"
#include "macrosconstants.h"
#include "macromanager.h"
#include "macro.h"
#include "macrosconstants.h"

#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/uniqueidmanager.h>

#include <QButtonGroup>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QCheckBox>
#include <QDir>
#include <QGroupBox>
#include <QHeaderView>
#include <QFileInfo>
#include <QRegExpValidator>
#include <QLineEdit>

namespace {
    int DIRECTORY = 1;
    int MACRO = 2;

    int DEFAULT_ROLE = Qt::UserRole;
    int NAME_ROLE = Qt::UserRole;
    int WRITE_ROLE = Qt::UserRole+1;
    int ID_ROLE = Qt::UserRole+2;
}

using namespace Macros;
using namespace Macros::Internal;


MacroOptionsWidget::MacroOptionsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MacroOptionsWidget),
    changingCurrent(false)
{
    ui->setupUi(this);

    connect(ui->treeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
            this, SLOT(changeCurrentItem(QTreeWidgetItem*)));
    connect(ui->removeButton, SIGNAL(clicked()),
            this, SLOT(remove()));
    connect(ui->defaultButton, SIGNAL(clicked()),
            this, SLOT(setDefault()));
    connect(ui->addButton, SIGNAL(clicked()),
            this, SLOT(addDirectoy()));
    connect(ui->description, SIGNAL(textChanged(QString)),
            this, SLOT(changeDescription(QString)));
    connect(ui->assignShortcut, SIGNAL(toggled(bool)),
            this, SLOT(changeShortcut(bool)));

    ui->treeWidget->header()->setSortIndicator(0, Qt::AscendingOrder);
}

MacroOptionsWidget::~MacroOptionsWidget()
{
    delete ui;
}

void MacroOptionsWidget::setSettings(const MacroSettings &s)
{
    m_macroToRemove.clear();
    m_macroToChange.clear();
    m_directories.clear();
    ui->treeWidget->clear();

    ui->showSaveDialog->setChecked(s.showSaveDialog);

    // Create the treeview
    foreach (const QString &dir, s.directories)
        appendDirectory(dir, s.defaultDirectory==dir);
    m_directories = s.directories;
}

void MacroOptionsWidget::appendDirectory(const QString &directory, bool isDefault)
{
    QDir dir(directory);
    QTreeWidgetItem *dirItem = new QTreeWidgetItem(ui->treeWidget, DIRECTORY);
    dirItem->setText(0, dir.absolutePath());
    dirItem->setData(0, DEFAULT_ROLE, isDefault);
    dirItem->setFirstColumnSpanned(true);
    if (isDefault)
        dirItem->setIcon(0, QPixmap(":/extensionsystem/images/ok.png"));

    Core::ICore *core = Core::ICore::instance();
    Core::ActionManager *am = core->actionManager();
    QMapIterator<QString, Macro *> it(MacroManager::instance()->macros());
    while (it.hasNext()) {
        it.next();
        QFileInfo fileInfo(it.value()->fileName());
        if (fileInfo.absoluteDir() == dir.absolutePath()) {
            QTreeWidgetItem *macroItem = new QTreeWidgetItem(dirItem, MACRO);
            macroItem->setText(0, it.value()->displayName());
            macroItem->setText(1, it.value()->description());
            macroItem->setData(0, NAME_ROLE, it.value()->displayName());
            macroItem->setData(0, WRITE_ROLE, it.value()->isWritable());
            macroItem->setData(0, ID_ROLE, it.value()->shortcutId());

            if (it.value()->shortcutId() >= 0) {
                QString textId = QString("%1").arg(it.value()->shortcutId(), 2, 10, QLatin1Char('0'));
                QString commandId = QLatin1String(Constants::SHORTCUT_MACRO)+textId;
                QString shortcut = am->command(commandId)->keySequence().toString();
                macroItem->setText(2, QString("%1 (%2)").arg(shortcut).arg(commandId));
            }
        }
    }
}

void MacroOptionsWidget::addDirectoy()
{
    QString directory = ui->directoryPathChooser->path();
    appendDirectory(directory, ui->treeWidget->children().count()==0);
}

void MacroOptionsWidget::changeCurrentItem(QTreeWidgetItem *current)
{
    changingCurrent = true;
    if (!current) {
        ui->removeButton->setEnabled(false);
        ui->defaultButton->setEnabled(false);
        ui->description->clear();
        ui->assignShortcut->setChecked(false);
        ui->macroGroup->setEnabled(false);
    }
    else if (current->type() == DIRECTORY) {
        bool isDefault = current->data(0, DEFAULT_ROLE).toBool();
        ui->removeButton->setEnabled(!isDefault);
        ui->defaultButton->setEnabled(!isDefault);
        ui->description->clear();
        ui->assignShortcut->setChecked(false);
        ui->macroGroup->setEnabled(false);
    } else {
        ui->removeButton->setEnabled(true);
        ui->defaultButton->setEnabled(false);
        ui->description->setText(current->text(1));
        ui->description->setEnabled(current->data(0, WRITE_ROLE).toBool());
        ui->assignShortcut->setChecked(current->data(0, ID_ROLE).toInt() >= 0);
        ui->macroGroup->setEnabled(true);
    }
    changingCurrent = false;
}

void MacroOptionsWidget::remove()
{
    QTreeWidgetItem *current = ui->treeWidget->currentItem();
    if (current->type() == MACRO)
        m_macroToRemove.append(current->text(0));
    delete current;
}

void MacroOptionsWidget::setDefault()
{
    QTreeWidgetItem *current = ui->treeWidget->currentItem();
    if (!current || current->type() != DIRECTORY)
        return;

    for (int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = ui->treeWidget->topLevelItem(i);
        if (item->data(0, DEFAULT_ROLE).toBool()) {
            item->setIcon(0, QPixmap());
            item->setData(0, DEFAULT_ROLE, false);
        }
    }

    current->setData(0, DEFAULT_ROLE, true);
    current->setIcon(0, QPixmap(":/extensionsystem/images/ok.png"));
}

void MacroOptionsWidget::apply()
{
    // Remove macro
    foreach (const QString &name, m_macroToRemove)
        MacroManager::instance()->deleteMacro(name);

    // Change macro
    QMapIterator<QString, ChangeSet> it(m_macroToChange);
    while (it.hasNext()) {
        it.next();
        MacroManager::instance()->changeMacro(it.key(), it.value().description,
                                              it.value().shortcut);
    }

    // Get list of dir to append or remove
    QStringList dirToAppend;
    QStringList dirToRemove = m_directories;
    for (int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = ui->treeWidget->topLevelItem(i);
        if (!m_directories.contains(item->text(0)))
            dirToAppend.append(item->text(0));
        dirToRemove.removeAll(item->text(0));
        if (item->data(0, DEFAULT_ROLE).toBool())
            MacroManager::instance()->setDefaultDirectory(item->text(0));
    }

    // Append/remove directory
    foreach (const QString &dir, dirToAppend)
        MacroManager::instance()->appendDirectory(dir);
    foreach (const QString &dir, dirToRemove)
        MacroManager::instance()->removeDirectory(dir);

    MacroManager::instance()->showSaveDialog(ui->showSaveDialog->checkState()==Qt::Checked);

    MacroManager::instance()->saveSettings();

    // Reinitialize the page
    setSettings(MacroManager::instance()->settings());
}

void MacroOptionsWidget::changeData(QTreeWidgetItem *current, int column, QVariant value)
{
    QString macroName = current->data(0, NAME_ROLE).toString();
    if (!m_macroToChange.contains(macroName)) {
        m_macroToChange[macroName].description = current->text(1);
        m_macroToChange[macroName].shortcut = (current->data(0, ID_ROLE).toInt() >= 0);
    }

    // Change the description
    if (column == 1) {
        m_macroToChange[macroName].description = value.toString();
        current->setText(1, value.toString());
        QFont font = current->font(1);
        font.setItalic(true);
        current->setFont(1, font);
    }
    // Change the shortcut
    if (column == 2) {
        bool shortcut = value.toBool();
        m_macroToChange[macroName].shortcut = shortcut;
        QFont font = current->font(2);
        if (current->data(0, ID_ROLE).toInt() >= 0) {
            font.setStrikeOut(!shortcut);
            font.setItalic(!shortcut);
        }
        else {
            font.setItalic(shortcut);
            current->setText(2, shortcut?tr("create shortcut"):"");
        }
        current->setFont(2, font);
    }
}

void MacroOptionsWidget::changeDescription(const QString &description)
{
    QTreeWidgetItem *current = ui->treeWidget->currentItem();
    if (changingCurrent || !current || current->type() == DIRECTORY)
        return;
    changeData(current, 1, description);
}

void MacroOptionsWidget::changeShortcut(bool shortcut)
{
    QTreeWidgetItem *current = ui->treeWidget->currentItem();
    if (changingCurrent || !current || current->type() == DIRECTORY)
        return;
    changeData(current, 2, shortcut);
}
