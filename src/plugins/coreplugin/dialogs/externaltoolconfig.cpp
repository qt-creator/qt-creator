/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "externaltoolconfig.h"
#include "ui_externaltoolconfig.h"

#include <utils/qtcassert.h>

#include <QtCore/QTextStream>

using namespace Core::Internal;

ExternalToolConfig::ExternalToolConfig(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ExternalToolConfig)
{
    ui->setupUi(this);
    ui->toolTree->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    connect(ui->toolTree, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
            this, SLOT(handleCurrentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
    connect(ui->toolTree, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
            this, SLOT(updateItemName(QTreeWidgetItem *)));

    showInfoForItem(0);
}

ExternalToolConfig::~ExternalToolConfig()
{
    QMapIterator<QString, QList<ExternalTool *> > it(m_tools);
    while (it.hasNext()) {
        it.next();
        qDeleteAll(it.value());
    }
    delete ui;
}

QString ExternalToolConfig::searchKeywords() const
{
    QString keywords;
    QTextStream(&keywords)
            << ui->descriptionLabel->text()
            << ui->executableLabel->text()
            << ui->argumentsLabel->text()
            << ui->workingDirectoryLabel->text()
            << ui->outputLabel->text()
            << ui->errorOutputLabel->text()
            << ui->modifiesDocumentCheckbox->text()
            << ui->inputLabel->text();
    return keywords;
}

void ExternalToolConfig::setTools(const QMap<QString, QList<ExternalTool *> > &tools)
{
    QMapIterator<QString, QList<ExternalTool *> > it(tools);
    while (it.hasNext()) {
        it.next();
        QList<ExternalTool *> itemCopy;
        foreach (ExternalTool *tool, it.value())
            itemCopy.append(new ExternalTool(tool));
        m_tools.insert(it.key(), itemCopy);
    }

    bool blocked = ui->toolTree->blockSignals(true); // block itemChanged
    Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsEditable;
    QMapIterator<QString, QList<ExternalTool *> > categories(m_tools);
    while (categories.hasNext()) {
        categories.next();
        QString name = (categories.key().isEmpty() ? tr("External Tools Menu") : categories.key());
        QTreeWidgetItem *category = new QTreeWidgetItem(ui->toolTree, QStringList() << name);
        category->setFlags(flags);
        category->setData(0, Qt::UserRole, name); // save the name in case of category being renamed by user
        foreach (ExternalTool *tool, categories.value()) {
            QTreeWidgetItem *item = new QTreeWidgetItem(category, QStringList() << tool->displayName());
            item->setFlags(flags);
            item->setData(0, Qt::UserRole, qVariantFromValue(tool));
        }
    }
    ui->toolTree->expandAll();
    ui->toolTree->blockSignals(blocked); // unblock itemChanged
}

void ExternalToolConfig::handleCurrentItemChanged(QTreeWidgetItem *now, QTreeWidgetItem *previous)
{
    updateItem(previous);
    showInfoForItem(now);
}

void ExternalToolConfig::updateItem(QTreeWidgetItem *item)
{
    ExternalTool *tool = 0;
    if (item)
        tool = item->data(0, Qt::UserRole).value<ExternalTool *>();
    if (!tool)
        return;
    tool->setDescription(ui->description->text());
    QStringList executables = tool->executables();
    if (executables.size() > 0)
        executables[0] = ui->executable->path();
    else
        executables << ui->executable->path();
    tool->setExecutables(executables);
    tool->setArguments(ui->arguments->text());
    tool->setWorkingDirectory(ui->workingDirectory->path());
    tool->setOutputHandling((ExternalTool::OutputHandling)ui->outputBehavior->currentIndex());
    tool->setErrorHandling((ExternalTool::OutputHandling)ui->errorOutputBehavior->currentIndex());
    tool->setModifiesCurrentDocument(ui->modifiesDocumentCheckbox->checkState());
    tool->setInput(ui->inputText->toPlainText());
}

void ExternalToolConfig::showInfoForItem(QTreeWidgetItem *item)
{
    ExternalTool *tool = 0;
    if (item)
        tool = item->data(0, Qt::UserRole).value<ExternalTool *>();
    if (!tool) {
        ui->description->setText(QString());
        ui->executable->setPath(QString());
        ui->arguments->setText(QString());
        ui->workingDirectory->setPath(QString());
        ui->inputText->setPlainText(QString());
        ui->infoWidget->setEnabled(false);
        return;
    }
    ui->infoWidget->setEnabled(true);
    ui->description->setText(tool->description());
    ui->executable->setPath(tool->executables().isEmpty() ? QString() : tool->executables().first());
    ui->arguments->setText(tool->arguments());
    ui->workingDirectory->setPath(tool->workingDirectory());
    ui->outputBehavior->setCurrentIndex((int)tool->outputHandling());
    ui->errorOutputBehavior->setCurrentIndex((int)tool->errorHandling());
    ui->modifiesDocumentCheckbox->setChecked(tool->modifiesCurrentDocument());
    ui->inputText->setPlainText(tool->input());
    ui->description->setCursorPosition(0);
    ui->arguments->setCursorPosition(0);
}

void ExternalToolConfig::updateItemName(QTreeWidgetItem *item)
{
    ExternalTool *tool = 0;
    if (item)
        tool = item->data(0, Qt::UserRole).value<ExternalTool *>();
    if (tool) {
        // tool was renamed
        const QString &newName = item->data(0, Qt::DisplayRole).toString();
        if (newName == tool->displayName())
            return;
        if (newName.isEmpty()) {
            // prevent empty names
            bool blocked = ui->toolTree->blockSignals(true); // block itemChanged
            item->setData(0, Qt::DisplayRole, tool->displayName());
            ui->toolTree->blockSignals(blocked); // unblock itemChanged
            return;
        }
        tool->setDisplayName(item->data(0, Qt::DisplayRole).toString());
    } else {
        // category was renamed
        const QString &oldName = item->data(0, Qt::UserRole).toString();
        const QString &newName = item->data(0, Qt::DisplayRole).toString();
        if (oldName == newName)
            return;
        if (newName.isEmpty() && m_tools.contains(newName)) {
            // prevent empty or duplicate names
            bool blocked = ui->toolTree->blockSignals(true); // block itemChanged
            item->setData(0, Qt::DisplayRole, oldName);
            ui->toolTree->blockSignals(blocked); // unblock itemChanged
            return;
        }
        QTC_ASSERT(m_tools.contains(oldName), return);
        m_tools.insert(newName, m_tools.value(oldName));
        m_tools.remove(oldName);

        bool blocked = ui->toolTree->blockSignals(true); // block itemChanged
        item->setData(0, Qt::UserRole, newName);
        int currentIndex = ui->toolTree->indexOfTopLevelItem(item);
        bool wasExpanded = ui->toolTree->isExpanded(ui->toolTree->model()->index(currentIndex, 0));
        ui->toolTree->takeTopLevelItem(currentIndex);
        int newIndex = m_tools.keys().indexOf(newName);
        ui->toolTree->insertTopLevelItem(newIndex, item);
        if (wasExpanded)
            ui->toolTree->expand(ui->toolTree->model()->index(newIndex, 0));
        ui->toolTree->blockSignals(blocked); // unblock itemChanged
    }
}

QMap<QString, QList<ExternalTool *> > ExternalToolConfig::tools() const
{
    return m_tools;
}

void ExternalToolConfig::apply()
{
    updateItem(ui->toolTree->currentItem());
}
