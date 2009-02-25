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

#include "proxml.h"
#include "proitems.h"
#include "proeditormodel.h"
#include "procommandmanager.h"
#include "proiteminfo.h"

#include <QtCore/QDebug>
#include <QtCore/QMimeData>
#include <QtCore/QStringList>
#include <QtGui/QIcon>

using namespace Qt4ProjectManager::Internal;

namespace Qt4ProjectManager {
namespace Internal {
    
class ProAddCommand : public ProCommand
{
public:
    ProAddCommand(ProEditorModel *model, ProItem *item, int row, const QModelIndex &parent, bool dodelete = true)
        : m_model(model), m_item(item), m_row(row), m_parent(parent), m_dodelete(dodelete), m_delete(false) { }

    ~ProAddCommand() {
        if (m_delete)
            delete m_item;
    }

    bool redo() {
        m_delete = false;
        return m_model->insertModelItem(m_item, m_row, m_parent);
    }

    void undo() {
        m_delete = m_dodelete;
        m_model->removeModelItem(m_model->index(m_row, 0, m_parent));
    }

private:
    ProEditorModel *m_model;
    ProItem *m_item;
    int m_row;
    const QModelIndex m_parent;
    bool m_dodelete;
    bool m_delete;
};

class ProRemoveCommand : public ProCommand
{
public:
    ProRemoveCommand(ProEditorModel *model, const QModelIndex &index, bool dodelete = true)
        : m_model(model), m_index(index), m_dodelete(dodelete), m_delete(dodelete) { }

    ~ProRemoveCommand() {
        if (m_delete)
            delete m_model->proItem(m_index);
    }

    bool redo() {
        m_delete = m_dodelete;
        return m_model->removeModelItem(m_index);
    }

    void undo() {
        m_delete = false;
        m_model->insertModelItem(m_model->proItem(m_index),
            m_index.row(), m_index.parent());
    }

private:
    ProEditorModel *m_model;
    const QModelIndex m_index;
    bool m_dodelete;
    bool m_delete;
};

class ChangeProVariableIdCommand : public ProCommand
{
public:
    ChangeProVariableIdCommand(ProEditorModel *model, ProVariable *variable, const QString &newId)
        : m_newId(newId), m_model(model), m_variable(variable)
    {
        m_oldId = m_variable->variable();
    }

    ~ChangeProVariableIdCommand() { }

    bool redo() {
        m_variable->setVariable(m_newId);
        return true;
    }

    void undo() {
        m_variable->setVariable(m_oldId);
    }

private:
    QString m_oldId;
    QString m_newId;

    ProEditorModel *m_model;
    ProVariable *m_variable;
};

class ChangeProVariableOpCommand : public ProCommand
{
public:
    ChangeProVariableOpCommand(ProEditorModel *model, ProVariable *variable, ProVariable::VariableOperator newOp)
        :  m_newOp(newOp), m_model(model), m_variable(variable)
    {
        m_oldOp = m_variable->variableOperator();
    }

    ~ChangeProVariableOpCommand() { }

    bool redo() {
        m_variable->setVariableOperator(m_newOp);
        return true;
    }

    void undo() {
        m_variable->setVariableOperator(m_oldOp);
    }

private:
    ProVariable::VariableOperator m_oldOp;
    ProVariable::VariableOperator m_newOp;

    ProEditorModel *m_model;
    ProVariable *m_variable;
};

class ChangeProScopeCommand : public ProCommand
{
public:
    ChangeProScopeCommand(ProEditorModel *model, ProBlock *scope, const QString &newExp)
        : m_newExp(newExp), m_model(model), m_scope(scope) {
        m_oldExp = m_model->expressionToString(m_scope);
    }

    ~ChangeProScopeCommand() { }

    bool redo() {
        setScopeCondition(m_newExp);
        return true;
    }

    void undo() {
        setScopeCondition(m_oldExp);
    }

private:
    void setScopeCondition(const QString &exp) {
        ProItem *contents = m_model->scopeContents(m_scope);
        QList<ProItem *> items = m_scope->items();
        for (int i=items.count() - 1; i>=0; --i) {
            if (items.at(i) != contents)
                delete items[i];
        }

        items = m_model->stringToExpression(exp);
        items << contents;
        m_scope->setItems(items);
    }

    QString m_oldExp;
    QString m_newExp;

    ProEditorModel *m_model;
    ProBlock *m_scope;
};

class ChangeProAdvancedCommand : public ProCommand
{
public:
    ChangeProAdvancedCommand(ProEditorModel *model, ProBlock *block, const QString &newExp)
        : m_newExp(newExp), m_model(model), m_block(block) {
        m_oldExp = m_model->expressionToString(m_block);
    }

    ~ChangeProAdvancedCommand() { }

    bool redo() {
        setExpression(m_newExp);
        return true;
    }

    void undo() {
        setExpression(m_oldExp);
    }

private:
    void setExpression(const QString &exp) {
        qDeleteAll(m_block->items());
        m_block->setItems(m_model->stringToExpression(exp));
    }

    QString m_oldExp;
    QString m_newExp;

    ProEditorModel *m_model;
    ProBlock *m_block;
};

} //namespace Internal
} //namespace Qt4ProjectManager

ProEditorModel::ProEditorModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    m_infomanager = 0;
    m_cmdmanager = new ProCommandManager(this);
}

ProEditorModel::~ProEditorModel()
{
}

void ProEditorModel::setInfoManager(ProItemInfoManager *infomanager)
{
    m_infomanager = infomanager;
    reset();
}

ProItemInfoManager *ProEditorModel::infoManager() const
{
    return m_infomanager;
}

ProCommandManager *ProEditorModel::cmdManager() const
{
    return m_cmdmanager;
}

void ProEditorModel::setProFiles(QList<ProFile*> proFiles)
{
    m_changed.clear();
    m_proFiles = proFiles;
    reset();
}

QList<ProFile*> ProEditorModel::proFiles() const
{
    return m_proFiles;
}

QList<QModelIndex> ProEditorModel::findVariables(const QStringList &varnames, const QModelIndex &parent) const
{
    QList<QModelIndex> result;

    if (varnames.isEmpty())
        return result;

    if (ProVariable *var = proVariable(parent)) {
        if (varnames.contains(var->variable()))
            result << parent;
        return result;
    }

    for (int i=0; i<rowCount(parent); ++i) {
        result += findVariables(varnames, index(i, 0, parent));
    }

    return result;
}

QList<QModelIndex> ProEditorModel::findBlocks(const QModelIndex &parent) const
{
    QList<QModelIndex> result;

    if (proBlock(parent)) {
        result << parent;
        return result;
    }

    for (int i = 0; i < rowCount(parent); ++i)
        result += findBlocks(index(i, 0, parent));

    return result;
}

QString ProEditorModel::blockName(ProBlock *block) const
{
    // variables has a name
    if (block->blockKind() & ProBlock::VariableKind) {
        ProVariable *v = static_cast<ProVariable*>(block);
        if (m_infomanager) {
            if (ProVariableInfo *info = m_infomanager->variable(v->variable()))
                return info->name();
        }
        return v->variable();
    }

    return expressionToString(block, true);
}

QModelIndex ProEditorModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row < 0 || (column != 0))
        return QModelIndex();

    if (parent.isValid()) {
        ProItem *item = proItem(parent);
        if (item->kind() != ProItem::BlockKind)
            return QModelIndex();

        ProBlock *block = static_cast<ProBlock*>(item);
        if (block->blockKind() & ProBlock::VariableKind
            || block->blockKind() & ProBlock::ProFileKind) {
            const QList<ProItem*> items = block->items();
            if (row >= items.count())
                return QModelIndex();
            ProItem *data = items.at(row);
            return createIndex(row, 0, (void*)data);
        } else if (ProBlock *scope = scopeContents(block)) {
            const QList<ProItem*> items = scope->items();
            if (row >= items.count())
                return QModelIndex();
            ProItem *data = items.at(row);
            return createIndex(row, 0, (void*)data);
        }

        return QModelIndex();
    }

    if (row >= m_proFiles.count())
        return QModelIndex();
    ProItem *data = m_proFiles.at(row);
    return createIndex(row, 0, (void*)data);
}

QModelIndex ProEditorModel::parent(const QModelIndex &index) const
{
    ProBlock *p = 0;
    ProItem *item = proItem(index);
    if (!item) {
        return QModelIndex();
    }

    if (item->kind() == ProItem::BlockKind) {
        ProBlock *block = static_cast<ProBlock *>(item);
        if (block->blockKind() & ProBlock::ProFileKind) {
            return QModelIndex();
        }
        p = block->parent();
    } else if (item->kind() == ProItem::ValueKind) {
        p = static_cast<ProValue *>(item)->variable();
    }

    if (p->blockKind() & ProBlock::ScopeContentsKind)
        p = p->parent();

    int row = -1;
    if (p->blockKind() & ProBlock::ProFileKind) {
        row = m_proFiles.indexOf(static_cast<ProFile*>(p));
    } else {
        ProBlock *pp = p->parent();
        row = pp->items().indexOf(p);
    }

    if (row == -1) {
        return QModelIndex();
    }

    ProItem *data = p;
    return createIndex(row, 0, (void*)data);
}

int ProEditorModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        ProItem *s = proItem(parent);
        if (!s)
            return 0;

        if (s->kind() != ProItem::BlockKind)
            return 0;

        ProBlock *block = static_cast<ProBlock*>(s);

        if (block->blockKind() & ProBlock::VariableKind
            || block->blockKind() & ProBlock::ProFileKind) {
            int rows = block->items().count();
            return rows;
        }

        if (ProBlock *scope = scopeContents(block)) {
            int rows = scope->items().count();
            return rows;
        }

        return 0;
    }

    return m_proFiles.count();
}

int ProEditorModel::columnCount(const QModelIndex &) const
{
    return 1;
}

QVariant ProEditorModel::data(const QModelIndex &index, int role) const
{
    ProItem *item = proItem(index);
    if (!item) {
        return QVariant();
    }

    if (item->kind() == ProItem::BlockKind) {
        ProBlock *block = static_cast<ProBlock*>(item);
        if (block->blockKind() & ProBlock::ProFileKind) {
            ProFile *pf = static_cast<ProFile*>(item);
            if (role == Qt::DisplayRole) {
                if (m_proFiles.count() > 1)
                    return QVariant(pf->displayFileName());
                else
                    return QVariant(tr("<Global Scope>"));
            } else if (role == Qt::DecorationRole) {
                return QIcon(":/proparser/images/profile.png");
            }
        } else if (block->blockKind() & ProBlock::ScopeKind) {
            if (role == Qt::DisplayRole)
                return QVariant(blockName(block));
            else if (role == Qt::DecorationRole)
                return QIcon(":/proparser/images/scope.png");
            else if (role == Qt::EditRole)
                return QVariant(expressionToString(block));
        } else if (block->blockKind() & ProBlock::VariableKind) {
            ProVariable *var = static_cast<ProVariable *>(block);
            if (role == Qt::DisplayRole) {
                return QVariant(blockName(block));
            } else if (role == Qt::DecorationRole) {
                if (var->variableOperator() == ProVariable::AddOperator)
                    return QIcon(":/proparser/images/append.png");
                else if (var->variableOperator() == ProVariable::RemoveOperator)
                    return QIcon(":/proparser/images/remove.png");
                else
                    return QIcon(":/proparser/images/set.png");
            } else if (role == Qt::EditRole) {
                return QVariant(var->variable());
            }
        } else {
            if (role == Qt::DisplayRole)
                return QVariant(blockName(block));
            else if (role == Qt::DecorationRole)
                return QIcon(":/proparser/images/other.png");
            else if (role == Qt::EditRole)
                return QVariant(expressionToString(block));
        }
    } else if (item->kind() == ProItem::ValueKind) {
        ProValue *value = static_cast<ProValue*>(item);
        if (role == Qt::DisplayRole) {
            ProVariable *var = proVariable(index.parent());
            if (var && m_infomanager) {
                if (ProVariableInfo *varinfo = m_infomanager->variable(var->variable())) {
                    if (ProValueInfo *valinfo = varinfo->value(value->value()))
                        return QVariant(valinfo->name());
                }
            }
            return QVariant(value->value());
        } else if (role == Qt::DecorationRole) {
            return QIcon(":/proparser/images/value.png");
        } else if (role == Qt::EditRole) {
            return QVariant(value->value());
        }
    }

    return QVariant();
}

bool ProEditorModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    static bool block = false;

    if (block)
        return false;

    if (role != Qt::EditRole)
        return false;

    ProItem *item = proItem(index);
    if (!item)
        return false;

    if (item->kind() == ProItem::ValueKind) {
        ProValue *val = static_cast<ProValue *>(item);
        if (val->value() == value.toString())
            return false;

        block = true;
        m_cmdmanager->beginGroup(tr("Change Item"));
        bool result = m_cmdmanager->command(new ProRemoveCommand(this, index));
        if (result) {
            ProValue *item = new ProValue(value.toString(), proVariable(index.parent()));
            result = m_cmdmanager->command(new ProAddCommand(this, item, index.row(), index.parent()));
        }
        block = false;

        m_cmdmanager->endGroup();
        markProFileModified(index);
        emit dataChanged(index,index);
        return result;
    } else if (item->kind() == ProItem::BlockKind) {
        ProBlock *block = proBlock(index);
        if (block->blockKind() & ProBlock::VariableKind) {
            ProVariable *var = static_cast<ProVariable *>(block);
            if (value.type() == QVariant::Int) {
                if ((int)var->variableOperator() == value.toInt())
                    return false;

                m_cmdmanager->beginGroup(tr("Change Variable Assignment"));
                m_cmdmanager->command(new ChangeProVariableOpCommand(this, var,
                    (ProVariable::VariableOperator)value.toInt()));
                m_cmdmanager->endGroup();
                markProFileModified(index);
                emit dataChanged(index,index);
                return true;
            } else {
                if (var->variable() == value.toString())
                    return false;

                m_cmdmanager->beginGroup(tr("Change Variable Type"));
                m_cmdmanager->command(new ChangeProVariableIdCommand(this, var,
                    value.toString()));
                m_cmdmanager->endGroup();
                markProFileModified(index);
                emit dataChanged(index,index);
                return true;
            }
        } else if (block->blockKind() & ProBlock::ScopeContentsKind) {
            ProBlock *scope = block->parent();
            QString oldExp = expressionToString(scope);
            if (oldExp == value.toString())
                return false;

            m_cmdmanager->beginGroup(tr("Change Scope Condition"));
            m_cmdmanager->command(new ChangeProScopeCommand(this, scope, value.toString()));
            m_cmdmanager->endGroup();
            markProFileModified(index);
            emit dataChanged(index,index);
            return true;
        } else if (block->blockKind() & ProBlock::ProFileKind) {
            return false;
        } else {
            QString oldExp = expressionToString(block);
            if (oldExp == value.toString())
                return false;

            m_cmdmanager->beginGroup(tr("Change Expression"));
            m_cmdmanager->command(new ChangeProAdvancedCommand(this, block, value.toString()));
            m_cmdmanager->endGroup();
            markProFileModified(index);
            emit dataChanged(index,index);
            return true;
        }
    }

    return false;
}

Qt::ItemFlags ProEditorModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    Qt::ItemFlags res = QAbstractItemModel::flags(index);
    ProItem *item = proItem(index);
    if (item->kind() == ProItem::BlockKind) {
        ProBlock *block = static_cast<ProBlock*>(item);
        if (block->blockKind() == ProBlock::ProFileKind)
            return res;
    }

    return res | Qt::ItemIsEditable;
}

QMimeData *ProEditorModel::mimeData(const QModelIndexList &indexes) const
{
    QModelIndex index = indexes.first();
    ProItem *item = proItem(index);
    QMimeData *data = new QMimeData();
    QString xml = ProXmlParser::itemToString(item);
    data->setText(xml);
    return data;
}

bool ProEditorModel::moveItem(const QModelIndex &index, int row)
{
    if (!index.isValid())
        return false;

    int oldrow = index.row();
    QModelIndex parentIndex = index.parent();

    if (oldrow == row)
        return false;

    ProItem *item = proItem(index);

    m_cmdmanager->beginGroup(tr("Move Item"));
    bool result = m_cmdmanager->command(new ProRemoveCommand(this, index, false));
    if (result)
        result = m_cmdmanager->command(new ProAddCommand(this, item, row, parentIndex, false));
    m_cmdmanager->endGroup();
    markProFileModified(index);

    return result;
}

bool ProEditorModel::removeModelItem(const QModelIndex &index)
{
    if (!index.isValid())
        return false;

    int row = index.row();
    QModelIndex parentIndex = index.parent();

    if (!parentIndex.isValid())
        return false;

    // get the pro items
    ProBlock *block = proBlock(parentIndex);
    if (!block)
        return false;

    QList<ProItem *> proitems = block->items();
    proitems.takeAt(row);

    beginRemoveRows(parentIndex, row, row);
    block->setItems(proitems);
    endRemoveRows();
    markProFileModified(index);

    return true;
}

bool ProEditorModel::removeItem(const QModelIndex &index)
{
    bool creategroup = !m_cmdmanager->hasGroup();
    if (creategroup)
        m_cmdmanager->beginGroup(tr("Remove Item"));

    bool result = m_cmdmanager->command(new ProRemoveCommand(this, index));

    if (creategroup)
        m_cmdmanager->endGroup();
    markProFileModified(index);
    return result;
}

bool ProEditorModel::insertModelItem(ProItem *item, int row, const QModelIndex &parent)
{
    if (!parent.isValid())
        return false;

    ProBlock *block = proBlock(parent);
    if (!item || !block)
        return false;

    QList<ProItem *> proitems = block->items();
    proitems.insert(row, item);

    if ((block->blockKind() & ProBlock::VariableKind)
        && item->kind() != ProItem::ValueKind)
        return false;

    if (item->kind() == ProItem::BlockKind) {
        static_cast<ProBlock*>(item)->setParent(block);
    } else if (item->kind() == ProItem::ValueKind) {
        if (!(block->blockKind() & ProBlock::VariableKind))
            return false;
        static_cast<ProValue*>(item)->
            setVariable(static_cast<ProVariable*>(block));
    } else {
        return false;
    }

    beginInsertRows(parent, row, row);
    block->setItems(proitems);
    endInsertRows();
    
    markProFileModified(parent);
    return true;
}

bool ProEditorModel::insertItem(ProItem *item, int row, const QModelIndex &parent)
{
    bool creategroup = !m_cmdmanager->hasGroup();
    if (creategroup)
        m_cmdmanager->beginGroup(tr("Insert Item"));

    bool result = m_cmdmanager->command(new ProAddCommand(this, item, row, parent));

    if (creategroup)
        m_cmdmanager->endGroup();
    markProFileModified(parent);
    return result;
}

void ProEditorModel::markProFileModified(QModelIndex index)
{
    while (index.isValid()) {        
        if (proItem(index)->kind() == ProItem::BlockKind) {
            ProBlock * block = proBlock(index);
            if (block->blockKind() == ProBlock::ProFileKind) {
                ProFile * file = static_cast<ProFile *>(block);
                file->setModified(true);
                return;
            }
        }
        index = index.parent();
    }
}

ProItem *ProEditorModel::proItem(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;
    return reinterpret_cast<ProItem*>(index.internalPointer());
}

ProVariable *ProEditorModel::proVariable(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    ProItem *item = proItem(index);
    if (item->kind() != ProItem::BlockKind)
        return 0;

    ProBlock *block = static_cast<ProBlock *>(item);
    if (block->blockKind() != ProBlock::VariableKind)
        return 0;

    return static_cast<ProVariable*>(block);
}

ProBlock *ProEditorModel::proBlock(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    ProItem *item = proItem(index);
    if (item->kind() != ProItem::BlockKind)
        return 0;

    ProBlock *block = static_cast<ProBlock *>(item);
    if (block->blockKind() & ProBlock::ScopeKind)
        block = scopeContents(block);

    return block;
}

QString ProEditorModel::expressionToString(ProBlock *block, bool display) const
{
    QString result;
    QList<ProItem*> items = block->items();
    for (int i = 0; i < items.count(); ++i) {
        ProItem *item = items.at(i);
        switch (item->kind()) {
            case ProItem::FunctionKind: {
                ProFunction *v = static_cast<ProFunction*>(item);
                result += v->text();
                break; }
            case ProItem::ConditionKind: {
                ProCondition *v = static_cast<ProCondition*>(item);
                if (m_infomanager && display) {
                    if (ProScopeInfo *info = m_infomanager->scope(v->text()))
                        result += info->name();
                    else
                        result += v->text();
                } else {
                    result += v->text();
                }
                break;
            }
            case ProItem::OperatorKind: {
                ProOperator *v = static_cast<ProOperator*>(item);
                if (v->operatorKind() == ProOperator::NotOperator)
                    result += QLatin1Char('!');
                else
                    result += QLatin1Char('|');
                break;
            }
            case ProItem::ValueKind:
            case ProItem::BlockKind:
                break; // ### unhandled
        }
    }

    return result;
}

ProItem *ProEditorModel::createExpressionItem(QString &str) const
{
    ProItem *item = 0;

    str = str.trimmed();
    if (str.endsWith(')'))
        item = new ProFunction(str);
    else if (!str.isEmpty())
        item = new ProCondition(str);

    str.clear();
    return item;
}


QList<ProItem *> ProEditorModel::stringToExpression(const QString &exp) const
{
    QList<ProItem*> result;
    int p = 0;
    bool c = false;

    QString tmpstr;
    for (int i = 0; i < exp.length(); ++i) {
        QChar tmpchar = exp.at(i);
        if (tmpchar == '(') ++p;
        else if (tmpchar == ')') --p;
        else if (tmpchar == '\'' || tmpchar == '\"') c = !c;
        else if (!c && !p) {
            if (tmpchar == '|') {
                if (ProItem *item = createExpressionItem(tmpstr))
                    result << item;
                result << new ProOperator(ProOperator::OrOperator);
                continue;
            } else if (tmpchar == '!') {
                if (ProItem *item = createExpressionItem(tmpstr))
                    result << item;
                result << new ProOperator(ProOperator::NotOperator);
                continue;
            }
        }
        tmpstr += tmpchar;
    }

    if (ProItem *item = createExpressionItem(tmpstr))
        result << item;

    return result;
}

ProBlock *ProEditorModel::scopeContents(ProBlock *block) const
{
    if (!(block->blockKind() & ProBlock::ScopeKind))
        return 0;

    ProItem *item = block->items().last();
    if (item->kind() != ProItem::BlockKind)
        return 0;
    ProBlock *scope = static_cast<ProBlock*>(item);
    if (!(scope->blockKind() & ProBlock::ScopeContentsKind))
        return 0;
    return scope;
}

ProScopeFilter::ProScopeFilter(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    m_checkable = ProScopeFilter::None;
}

void ProScopeFilter::setVariableFilter(const QStringList &vars)
{
    m_vars = vars;
}

void ProScopeFilter::setCheckable(CheckableType ct)
{
    m_checkable = ct;
}

QList<QModelIndex> ProScopeFilter::checkedIndexes() const
{
    return m_checkStates.keys(true);
}

Qt::ItemFlags ProScopeFilter::flags(const QModelIndex &index) const
{
    Qt::ItemFlags srcflags = sourceModel()->flags(mapToSource(index));
    srcflags &= ~Qt::ItemIsDragEnabled; //disable drag

    if (m_checkable == ProScopeFilter::None)
        return srcflags;

    return (srcflags|Qt::ItemIsUserCheckable);
}

QVariant ProScopeFilter::data(const QModelIndex &index, int role) const
{
    bool checkable =
        m_checkable == ProScopeFilter::Blocks
        || (m_checkable == ProScopeFilter::Variable && sourceVariable(index));

    if (checkable && role == Qt::CheckStateRole) {
        QModelIndex srcindex = mapToSource(index);
        if (m_checkStates.value(srcindex, false))
            return Qt::Checked;
        else
            return Qt::Unchecked;
    }

    return QSortFilterProxyModel::data(index, role);
}

bool ProScopeFilter::setData(const QModelIndex &index, const QVariant &value, int role)
{
    // map to source
    if (m_checkable != ProScopeFilter::None && role == Qt::CheckStateRole) {
        if (m_checkable == ProScopeFilter::Blocks
             || (m_checkable == ProScopeFilter::Variable && sourceVariable(index))) {
            QModelIndex srcindex = mapToSource(index);
            if (value.toInt() == Qt::Checked && !m_checkStates.value(srcindex, false)) {
                m_checkStates.insert(srcindex, true);
                emit dataChanged(index, index);
            } else if (m_checkStates.value(srcindex, true)) {
                m_checkStates.insert(srcindex, false);
                emit dataChanged(index, index);
            }
            return true;
       }
    }

    return QSortFilterProxyModel::setData(index, value, role);
}

ProVariable *ProScopeFilter::sourceVariable(const QModelIndex &index) const
{
    ProEditorModel *model = qobject_cast<ProEditorModel*>(sourceModel());
    return model->proVariable(mapToSource(index));
}

bool ProScopeFilter::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    ProEditorModel *model = qobject_cast<ProEditorModel*>(sourceModel());
    if (!model)
        return true;

    QModelIndex index = model->index(source_row, 0, source_parent);
    ProItem *item = model->proItem(index);
    if (item->kind() != ProItem::BlockKind)
        return false;

    ProBlock *block = static_cast<ProBlock *>(item);

    if (m_vars.isEmpty())
        return (block->blockKind() & ProBlock::ScopeKind || block->blockKind() & ProBlock::ProFileKind);

    if (block->blockKind() & ProBlock::VariableKind
        || block->blockKind() & ProBlock::ScopeKind
        || block->blockKind() & ProBlock::ProFileKind)
            return !model->findVariables(m_vars, index).isEmpty();

    return false;
}
