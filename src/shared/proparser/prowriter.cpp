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
#include "prowriter.h"

#include <QtCore/QFile>

using namespace Qt4ProjectManager::Internal;

bool ProWriter::write(ProFile *profile, const QString &fileName)
{
    QFile data(fileName);
    if (!data.open(QFile::WriteOnly|QFile::Text))
         return false;

    m_writeState = 0;
    m_comment.clear();
    m_out.setDevice(&data);
    writeItem(profile, QString());
    data.close();
    
    return true;
}

QString ProWriter::contents(ProFile *profile)
{
    QString result;

    m_writeState = 0;
    m_comment.clear();
    m_out.setString(&result, QIODevice::WriteOnly);
    writeItem(profile, QString());

    return result;
}

QString ProWriter::fixComment(const QString &comment, const QString &indent) const
{
    QString result = comment;
    result = result.replace(QLatin1Char('\n'), 
        QLatin1Char('\n') + indent + QLatin1String("# "));
    return QLatin1String("# ") + result;
}

void ProWriter::writeValue(ProValue *value, const QString &indent)
{
    if (m_writeState & NewLine) {
        m_out << indent << QLatin1String("    ");
        m_writeState &= ~NewLine;
    }
    
    m_out << value->value();

    if (!(m_writeState & LastItem))
        m_out << QLatin1String(" \\");

    if (!value->comment().isEmpty())
        m_out << QLatin1Char(' ') << fixComment(value->comment(), indent);

    m_out << endl;
    m_writeState |= NewLine;
}

void ProWriter::writeOther(ProItem *item, const QString &indent)
{
    if (m_writeState & NewLine) {
        m_out << indent;
        m_writeState &= ~NewLine;
    }

    if (item->kind() == ProItem::FunctionKind) {
        ProFunction *v = static_cast<ProFunction*>(item);
        m_out << v->text();
    } else  if (item->kind() == ProItem::ConditionKind) {
        ProCondition *v = static_cast<ProCondition*>(item);
        m_out << v->text();
    } else if (item->kind() == ProItem::OperatorKind) {
        ProOperator *v = static_cast<ProOperator*>(item);
        if (v->operatorKind() == ProOperator::OrOperator)
            m_out << QLatin1Char('|');
        else
            m_out << QLatin1Char('!');
    }

    if (!item->comment().isEmpty()) {
        if (!m_comment.isEmpty())
            m_comment += QLatin1Char('\n');
        m_comment += item->comment();
    }
}

void ProWriter::writeBlock(ProBlock *block, const QString &indent)
{
    if (m_writeState & NewLine) {
        m_out << indent;
        m_writeState &= ~NewLine;
    }

    if (!block->comment().isEmpty()) {
        if (!(m_writeState & FirstItem))
            m_out << endl << indent;
        m_out << fixComment(block->comment(), indent) << endl << indent;
    }

    QString newindent = indent;
    if (block->blockKind() & ProBlock::VariableKind) {
        ProVariable *v = static_cast<ProVariable*>(block);
        m_out << v->variable();
        switch (v->variableOperator()) {
            case ProVariable::AddOperator:
                m_out << QLatin1String(" += "); break;
            case ProVariable::RemoveOperator:
                m_out << QLatin1String(" -= "); break;
            case ProVariable::ReplaceOperator:
                m_out << QLatin1String(" ~= "); break;
            case ProVariable::SetOperator:
                m_out << QLatin1String(" = "); break;
            case ProVariable::UniqueAddOperator:
                m_out << QLatin1String(" *= "); break;
        }
    } else if (block->blockKind() & ProBlock::ScopeContentsKind) {
        if (block->items().count() > 1) {
            newindent = indent + QLatin1String("    ");
            m_out << QLatin1String(" { ");
            if (!m_comment.isEmpty()) {
                m_out << fixComment(m_comment, indent);
                m_comment.clear();
            }
            m_out << endl;
            m_writeState |= NewLine;
        } else {
            m_out << QLatin1Char(':');
        }
    }

    QList<ProItem*> items = block->items();
    for (int i = 0; i < items.count(); ++i) {
        m_writeState &= ~LastItem;
        m_writeState &= ~FirstItem;
        if (i == 0)
            m_writeState |= FirstItem;
        if (i == items.count() - 1)
            m_writeState |= LastItem;
        writeItem(items.at(i), newindent);
    }

    if ((block->blockKind() & ProBlock::ScopeContentsKind) && (block->items().count() > 1)) {
        if (m_writeState & NewLine) {
            m_out << indent;
            m_writeState &= ~NewLine;
        }
        m_out << QLatin1Char('}');
    }

    if (!m_comment.isEmpty()) {
        m_out << fixComment(m_comment, indent);
        m_out << endl;
        m_writeState |= NewLine;
        m_comment.clear();
    }

    if (!(m_writeState & NewLine)) {
        m_out << endl;
        m_writeState |= NewLine;
    }
}

void ProWriter::writeItem(ProItem *item, const QString &indent)
{
    if (item->kind() == ProItem::ValueKind) {
        writeValue(static_cast<ProValue*>(item), indent);
    } else if (item->kind() == ProItem::BlockKind) {
        writeBlock(static_cast<ProBlock*>(item), indent);
    } else {
        writeOther(item, indent);
    }
}
