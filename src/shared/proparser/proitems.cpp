/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "proitems.h"

#include <QtCore/QFileInfo>

QT_BEGIN_NAMESPACE

// --------------- ProBlock ----------------

ProBlock::ProBlock(ProBlock *parent)
{
    m_blockKind = 0;
    m_parent = parent;
    m_refCount = 1;
}

ProBlock::~ProBlock()
{
    foreach (ProItem *itm, m_proitems)
        if (itm->kind() == BlockKind)
            static_cast<ProBlock *>(itm)->deref();
        else
            delete itm;
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

// --------------- ProVariable ----------------
ProVariable::ProVariable(const QString &name)
{
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

void ProVariable::setValue(const QString &val)
{
    m_value = val;
}

QString ProVariable::value() const
{
    return m_value;
}

ProItem::ProItemKind ProVariable::kind() const
{
    return ProItem::VariableKind;
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

// --------------- ProFile ----------------
ProFile::ProFile(const QString &fileName)
    : ProBlock(0)
{
    setBlockKind(ProBlock::ProFileKind);
    m_fileName = fileName;

    // If the full name does not outlive the parts, things will go boom ...
    int nameOff = fileName.lastIndexOf(QLatin1Char('/'));
    m_displayFileName = QString::fromRawData(fileName.constData() + nameOff + 1,
                                             fileName.length() - nameOff - 1);
    m_directoryName = QString::fromRawData(fileName.constData(), nameOff);
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

QT_END_NAMESPACE
