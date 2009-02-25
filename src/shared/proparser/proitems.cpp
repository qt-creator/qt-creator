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

#include "proitems.h"
#include "abstractproitemvisitor.h"

#include <QtCore/QFileInfo>

QT_BEGIN_NAMESPACE

// --------------- ProItem ------------
void ProItem::setComment(const QString &comment)
{
    m_comment = comment;
}

QString ProItem::comment() const
{
    return m_comment;
}

// --------------- ProBlock ----------------
ProBlock::ProBlock(ProBlock *parent)
{
    m_blockKind = 0;
    m_parent = parent;
}

ProBlock::~ProBlock()
{
    qDeleteAll(m_proitems);
}

void ProBlock::appendItem(ProItem *proitem)
{
    m_proitems << proitem;
}

void ProBlock::setItems(const QList<ProItem *> &proitems)
{
    m_proitems = proitems;
}

QList<ProItem *> ProBlock::items() const
{
    return m_proitems;
}

void ProBlock::setBlockKind(int blockKind)
{
    m_blockKind = blockKind;
}

int ProBlock::blockKind() const
{
    return m_blockKind;
}

void ProBlock::setParent(ProBlock *parent)
{
    m_parent = parent;
}

ProBlock *ProBlock::parent() const
{
    return m_parent;
}

ProItem::ProItemKind ProBlock::kind() const
{
    return ProItem::BlockKind;
}

bool ProBlock::Accept(AbstractProItemVisitor *visitor)
{
    visitor->visitBeginProBlock(this);
    foreach (ProItem *item, m_proitems) {
        if (!item->Accept(visitor))
            return false;
    }
    return visitor->visitEndProBlock(this);
}

// --------------- ProVariable ----------------
ProVariable::ProVariable(const QString &name, ProBlock *parent)
    : ProBlock(parent)
{
    setBlockKind(ProBlock::VariableKind);
    m_variable = name;
    m_variableKind = SetOperator;
}

void ProVariable::setVariableOperator(VariableOperator variableKind)
{
    m_variableKind = variableKind;
}

ProVariable::VariableOperator ProVariable::variableOperator() const
{
    return m_variableKind;
}

void ProVariable::setVariable(const QString &name)
{
    m_variable = name;
}

QString ProVariable::variable() const
{
    return m_variable;
}

bool ProVariable::Accept(AbstractProItemVisitor *visitor)
{
    visitor->visitBeginProVariable(this);
    foreach (ProItem *item, m_proitems) {
        if (!item->Accept(visitor))
            return false;
    }
    return visitor->visitEndProVariable(this);
}

// --------------- ProValue ----------------
ProValue::ProValue(const QString &value, ProVariable *variable)
{
    m_variable = variable;
    m_value = value;
}

void ProValue::setValue(const QString &value)
{
    m_value = value;
}

QString ProValue::value() const
{
    return m_value;
}

void ProValue::setVariable(ProVariable *variable)
{
    m_variable = variable;
}

ProVariable *ProValue::variable() const
{
    return m_variable;
}

ProItem::ProItemKind ProValue::kind() const
{
    return ProItem::ValueKind;
}

bool ProValue::Accept(AbstractProItemVisitor *visitor)
{
    return visitor->visitProValue(this);
}

// --------------- ProFunction ----------------
ProFunction::ProFunction(const QString &text)
{
    m_text = text;
}

void ProFunction::setText(const QString &text)
{
    m_text = text;
}

QString ProFunction::text() const
{
    return m_text;
}

ProItem::ProItemKind ProFunction::kind() const
{
    return ProItem::FunctionKind;
}

bool ProFunction::Accept(AbstractProItemVisitor *visitor)
{
    return visitor->visitProFunction(this);
}

// --------------- ProCondition ----------------
ProCondition::ProCondition(const QString &text)
{
    m_text = text;
}

void ProCondition::setText(const QString &text)
{
    m_text = text;
}

QString ProCondition::text() const
{
    return m_text;
}

ProItem::ProItemKind ProCondition::kind() const
{
    return ProItem::ConditionKind;
}

bool ProCondition::Accept(AbstractProItemVisitor *visitor)
{
    return visitor->visitProCondition(this);
}

// --------------- ProOperator ----------------
ProOperator::ProOperator(OperatorKind operatorKind)
{
    m_operatorKind = operatorKind;
}

void ProOperator::setOperatorKind(OperatorKind operatorKind)
{
    m_operatorKind = operatorKind;
}

ProOperator::OperatorKind ProOperator::operatorKind() const
{
    return m_operatorKind;
}

ProItem::ProItemKind ProOperator::kind() const
{
    return ProItem::OperatorKind;
}

bool ProOperator::Accept(AbstractProItemVisitor *visitor)
{
    return visitor->visitProOperator(this);
}

// --------------- ProFile ----------------
ProFile::ProFile(const QString &fileName)
    : ProBlock(0)
{
    m_modified = false;
    setBlockKind(ProBlock::ProFileKind);
    m_fileName = fileName;

    QFileInfo fi(fileName);
    m_displayFileName = fi.fileName();
    m_directoryName = fi.absolutePath();
}

ProFile::~ProFile()
{
}

QString ProFile::displayFileName() const
{
    return m_displayFileName;
}

QString ProFile::fileName() const
{
    return m_fileName;
}

QString ProFile::directoryName() const
{
    return m_directoryName;
}

void ProFile::setModified(bool modified)
{
    m_modified = modified;
}

bool ProFile::isModified() const
{
    return m_modified;
}

bool ProFile::Accept(AbstractProItemVisitor *visitor)
{
    visitor->visitBeginProFile(this);
    foreach (ProItem *item, m_proitems) {
        if (!item->Accept(visitor))
            return false;
    }
    return visitor->visitEndProFile(this);
}

QT_END_NAMESPACE
