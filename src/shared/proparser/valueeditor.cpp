/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "valueeditor.h"
#include "proitems.h"
#include "proeditormodel.h"
#include "proiteminfo.h"

#include <QtGui/QMenu>
#include <QtGui/QKeyEvent>

using namespace Qt4ProjectManager::Internal;

ValueEditor::ValueEditor(QWidget *parent)
  : QWidget(parent),
    m_model(0),
    m_handleModelChanges(true),
    m_infomanager(0)
{
    setupUi(this);
}

ValueEditor::~ValueEditor()
{
}

void ValueEditor::initialize(ProEditorModel *model, ProItemInfoManager *infomanager)
{
    m_model = model;
    m_infomanager = infomanager;
    initialize();
}

void ValueEditor::hideVariable()
{
    m_varGroupBox->setVisible(false);
}

void ValueEditor::showVariable(bool advanced)
{
    m_varComboBoxLabel->setVisible(!advanced);
    m_varComboBox->setVisible(!advanced);

    m_varLineEditLabel->setVisible(advanced);
    m_varLineEdit->setVisible(advanced);

    m_assignComboBoxLabel->setVisible(advanced);
    m_assignComboBox->setVisible(advanced);

    m_varGroupBox->setVisible(true);
}

void ValueEditor::setItemEditType(ItemEditType type)
{
    m_editStackWidget->setCurrentIndex(type);
}

void ValueEditor::setDescription(ItemEditType type, const QString &header, const QString &description)
{
    switch (type) {
    case MultiUndefined:
        m_multiUndefinedGroupBox->setTitle(header);
        m_multiUndefinedDescriptionLabel->setVisible(!description.isEmpty());
        m_multiUndefinedDescriptionLabel->setText(description);
        break;
    case MultiDefined:
        m_multiDefinedGroupBox->setTitle(header);
        m_multiDefinedDescriptionLabel->setVisible(!description.isEmpty());
        m_multiDefinedDescriptionLabel->setText(description);
        break;
    case SingleUndefined:
        m_singleUndefinedGroupBox->setTitle(header);
        m_singleUndefinedDescriptionLabel->setVisible(!description.isEmpty());
        m_singleUndefinedDescriptionLabel->setText(description);
        break;
    default:
        m_singleDefinedGroupBox->setTitle(header);
        m_singleDefinedDescriptionLabel->setVisible(!description.isEmpty());
        m_singleDefinedDescriptionLabel->setText(description);
        break;
    }
}

void ValueEditor::initialize()
{
    hideVariable();
    setItemEditType(MultiUndefined);

    m_itemListView->setModel(m_model);
    m_itemListView->setRootIndex(QModelIndex());

    connect(m_itemAddButton, SIGNAL(clicked()),
        this, SLOT(addItem()));
    connect(m_itemRemoveButton, SIGNAL(clicked()),
        this, SLOT(removeItem()));

    connect(m_itemListView->selectionModel(),
        SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
        this, SLOT(updateItemList(const QModelIndex &)));

    connect(m_itemListWidget, SIGNAL(itemChanged(QListWidgetItem *)),
        this, SLOT(updateItemChanges(QListWidgetItem *)));

    foreach (ProVariableInfo *varinfo, m_infomanager->variables()) {
        m_varComboBox->addItem(varinfo->name(), varinfo->id());
    }

    connect(m_varLineEdit, SIGNAL(editingFinished()), this, SLOT(updateVariableId()));
    connect(m_varComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateVariableId(int)));
    connect(m_assignComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateVariableOp(int)));

    connect(m_itemLineEdit, SIGNAL(editingFinished()), this, SLOT(updateItemId()));
    connect(m_itemComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateItemId(int)));

    connect(m_model, SIGNAL(rowsInserted(const QModelIndex &, int, int)),
        this, SLOT(modelChanged(const QModelIndex &)));

    connect(m_model, SIGNAL(rowsRemoved(const QModelIndex &, int, int)),
        this, SLOT(modelChanged(const QModelIndex &)));

    connect(m_model, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
        this, SLOT(modelChanged(const QModelIndex &)));

    updateItemList(QModelIndex());
}

void ValueEditor::modelChanged(const QModelIndex &index)
{
    if (m_handleModelChanges) {
        if (m_currentIndex == index || m_currentIndex == index.parent())
            editIndex(m_currentIndex);
    }
}

void ValueEditor::editIndex(const QModelIndex &index)
{
    if (!m_model)
        return;
    m_currentIndex = index;
    ProBlock *block = m_model->proBlock(index);

    m_varGroupBox->setEnabled(block != 0);
    m_editStackWidget->setEnabled(block != 0);

    if (!block)
        return;

    if (block->blockKind() & ProBlock::ScopeContentsKind) {
        showScope(block);
    } else if (block->blockKind() & ProBlock::VariableKind) {
        showVariable(static_cast<ProVariable*>(block));
    } else {
        showOther(block);
    }
}

ValueEditor::ItemEditType ValueEditor::itemType(bool defined, bool multiple) const
{
    if (defined) {
        if (multiple)
            return MultiDefined;
        else
            return SingleDefined;
    } else {
        if (multiple)
            return MultiUndefined;
        else
            return SingleUndefined;
    }
}

void ValueEditor::showVariable(ProVariable *variable)
{
    if (!m_model)
        return;
    ProVariableInfo *info = m_infomanager->variable(variable->variable());

    const bool advanced = (!m_model->infoManager() || (info == 0));
    bool defined = false;
    bool multiple = true;

    QSet<QString> values;
    foreach(ProItem *proitem, variable->items()) {
        if (proitem->kind() == ProItem::ValueKind) {
            ProValue *val = static_cast<ProValue *>(proitem);
            values.insert(val->value());
        }
    }

    if (!advanced && info) {
        defined = !info->values().isEmpty();

        // check if all values are known
        foreach(QString val, values) {
            if (!info->value(val)) {
                defined = false;
                break;
            }
        }

        multiple = info->multiple();
    }

    if (values.count() > 1)
        multiple = true;

    bool wasblocked;

    if (!advanced) {
        const int index = m_varComboBox->findData(variable->variable(), Qt::UserRole, Qt::MatchExactly);
        wasblocked = m_varComboBox->blockSignals(true);
        m_varComboBox->setCurrentIndex(index);
        m_varComboBox->blockSignals(wasblocked);
    } else {
        wasblocked = m_varLineEdit->blockSignals(true);
        m_varLineEdit->setText(variable->variable());
        m_varLineEdit->blockSignals(wasblocked);
    }

    ItemEditType type = itemType(defined, multiple);

    wasblocked = m_assignComboBox->blockSignals(true);
    m_assignComboBox->setCurrentIndex(variable->variableOperator());
    m_assignComboBox->blockSignals(wasblocked);

    QString header = tr("Edit Values");
    QString desc;
    if (info) {
        header = tr("Edit %1").arg(info->name());
        desc = info->description();
    }
    setDescription(type, header, desc);

    m_itemListWidget->clear();

    switch (type) {
    case MultiUndefined: {
        const QModelIndex parent = m_currentIndex;
        m_itemListView->setRootIndex(parent);
        m_itemListView->setCurrentIndex(m_model->index(0,0,parent));
    }
        break;
    case MultiDefined:
        wasblocked = m_itemListWidget->blockSignals(true);

        foreach(ProValueInfo *valinfo, info->values()) {
            QListWidgetItem *item = new QListWidgetItem(m_itemListWidget);
            item->setText(valinfo->name());
            item->setData(Qt::UserRole, valinfo->id());

            if (values.contains(valinfo->id()))
                item->setCheckState(Qt::Checked);
            else
                item->setCheckState(Qt::Unchecked);
        }

        m_itemListWidget->blockSignals(wasblocked);
        break;
    case SingleUndefined:
        wasblocked = m_itemLineEdit->blockSignals(true);
        if (values.isEmpty())
            m_itemLineEdit->setText(QString());
        else
            m_itemLineEdit->setText(values.toList().first());
        m_itemLineEdit->blockSignals(wasblocked);
        break;
    case SingleDefined:
        wasblocked = m_itemComboBox->blockSignals(true);
        m_itemComboBox->clear();

        foreach(ProValueInfo *valinfo, info->values()) {
            m_itemComboBox->addItem(valinfo->name(), valinfo->id());
        }

        int index = -1;
        if (!values.isEmpty()) {
            const QString id = values.toList().first();
            index = m_itemComboBox->findData(id, Qt::UserRole, Qt::MatchExactly);
        }

        m_itemComboBox->setCurrentIndex(index);
        m_itemComboBox->blockSignals(wasblocked);
        break;
    }

    showVariable(advanced);
    setItemEditType(type);
}

void ValueEditor::showScope(ProBlock *)
{
    if (!m_model)
        return;
    const bool wasblocked = m_itemLineEdit->blockSignals(true);
    m_itemLineEdit->setText(m_model->data(m_currentIndex, Qt::EditRole).toString());
    m_itemLineEdit->blockSignals(wasblocked);

    setDescription(SingleUndefined, tr("Edit Scope"));

    hideVariable();
    setItemEditType(SingleUndefined);
}

void ValueEditor::showOther(ProBlock *)
{
    if (!m_model)
        return;
    const bool wasblocked = m_itemLineEdit->blockSignals(true);
    m_itemLineEdit->setText(m_model->data(m_currentIndex, Qt::EditRole).toString());
    m_itemLineEdit->blockSignals(wasblocked);

    setDescription(SingleUndefined, tr("Edit Advanced Expression"));

    hideVariable();
    setItemEditType(SingleUndefined);
}

void ValueEditor::addItem(QString value)
{
    if (!m_model)
        return;
    QModelIndex parent = m_currentIndex;
    ProVariable *var = static_cast<ProVariable *>(m_model->proBlock(parent));

    if (value.isEmpty()) {
        value = QLatin1String("...");

        if (ProVariableInfo *varinfo = m_infomanager->variable(var->variable())) {
            const QList<ProValueInfo *> vals = varinfo->values();
            if (!vals.isEmpty())
                value = vals.first()->id();
        }
    }

    m_handleModelChanges = false;
    m_model->insertItem(new ProValue(value, var),
        m_model->rowCount(parent), parent);

    const QModelIndex idx = m_model->index(m_model->rowCount(parent)-1, 0, parent);
    m_itemListView->setCurrentIndex(idx);
    m_itemListView->edit(idx);
    m_itemListView->scrollToBottom();
    m_handleModelChanges = true;
}

void ValueEditor::removeItem()
{
    if (!m_model)
        return;
    m_handleModelChanges = false;
    const QModelIndex idx = m_itemListView->currentIndex();
    m_itemListView->closePersistentEditor(idx);
    m_model->removeItem(idx);
    m_handleModelChanges = true;
}

void ValueEditor::updateItemList(const QModelIndex &)
{
    if (!m_model)
        return;
    m_itemRemoveButton->setEnabled(m_model->rowCount(m_currentIndex));
}

QModelIndex ValueEditor::findValueIndex(const QString &id) const
{
    if (!m_model)
        return QModelIndex();
    const QModelIndex parent = m_currentIndex;
    const int rows = m_model->rowCount(parent);

    for (int row=0; row<rows; ++row) {
        const QModelIndex index = m_model->index(row, 0, parent);
        ProItem *item = m_model->proItem(index);
        if (!item || item->kind() != ProItem::ValueKind)
            continue;

        if (static_cast<ProValue*>(item)->value() == id)
            return index;
    }

    return QModelIndex();
}

void ValueEditor::updateItemChanges(QListWidgetItem *item)
{
    if (!m_model)
        return;
    const QModelIndex parent = m_currentIndex;
    ProBlock *block = m_model->proBlock(parent);

    if (!block || !(block->blockKind() & ProBlock::VariableKind))
        return;

    ProVariable *var = static_cast<ProVariable *>(block);
    const QString id = item->data(Qt::UserRole).toString();

    m_handleModelChanges = false;
    const QModelIndex index = findValueIndex(id);
    if (item->checkState() == Qt::Checked && !index.isValid()) {
        m_model->insertItem(new ProValue(id, var),
            m_model->rowCount(parent), m_currentIndex);
    } else if (item->checkState() != Qt::Checked && index.isValid()) {
        m_model->removeItem(index);
    }
    m_handleModelChanges = true;
}

void ValueEditor::updateVariableId()
{
    if (!m_model)
        return;
    m_handleModelChanges = false;
    m_model->setData(m_currentIndex, QVariant(m_varLineEdit->text()));
    m_handleModelChanges = true;
}

void ValueEditor::updateVariableId(int index)
{
    if (!m_model)
        return;
    ProVariableInfo *info = m_infomanager->variable(m_varComboBox->itemData(index).toString());

    m_model->setData(m_currentIndex, info->id());
    m_model->setData(m_currentIndex, info->defaultOperator());
}

void ValueEditor::updateVariableOp(int index)
{
    if (!m_model)
        return;
    m_handleModelChanges = false;
    m_model->setData(m_currentIndex, QVariant(index));
    m_handleModelChanges = true;
}

void ValueEditor::updateItemId()
{
    if (!m_model)
        return;
    QModelIndex index = m_currentIndex;
    if (m_varGroupBox->isVisible()) {
        index = m_model->index(0,0,index);
        if (!index.isValid()) {
            addItem(m_itemLineEdit->text());
            return;
        }
    }

    m_handleModelChanges = false;
    m_model->setData(index, QVariant(m_itemLineEdit->text()));
    m_handleModelChanges = true;
}

void ValueEditor::updateItemId(int index)
{
    if (!m_model)
        return;
    QModelIndex idx = m_currentIndex;
    if (m_varGroupBox->isVisible()) {
        idx = m_model->index(0,0,idx);
        if (!idx.isValid()) {
            addItem(m_itemComboBox->itemData(index).toString());
            return;
        }
    }

    m_handleModelChanges = false;
    m_model->setData(idx, m_itemComboBox->itemData(index));
    m_handleModelChanges = true;
}
