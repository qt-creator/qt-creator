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

#include "disassemblerhandler.h"

#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QAbstractTableModel>

#include <QtGui/QIcon>

using namespace Debugger;
using namespace Debugger::Internal;


//////////////////////////////////////////////////////////////////
//
// DisassemblerModel
//
//////////////////////////////////////////////////////////////////

/*! A model to represent the stack in a QTreeView. */
class Debugger::Internal::DisassemblerModel : public QAbstractTableModel
{
public:
    DisassemblerModel(QObject *parent);

    // ItemModel
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const;

    // Properties
    void setLines(const QList<DisassemblerLine> &lines);
    QList<DisassemblerLine> lines() const;
    void setCurrentLine(int line) { m_currentLine = line; }

private:
    friend class DisassemblerHandler;
    QList<DisassemblerLine> m_lines;
    int m_currentLine;
    QIcon m_positionIcon;
    QIcon m_emptyIcon;
};

DisassemblerModel::DisassemblerModel(QObject *parent)
  : QAbstractTableModel(parent), m_currentLine(0)
{
    m_emptyIcon = QIcon(":/gdbdebugger/images/empty.svg");
    m_positionIcon = QIcon(":/gdbdebugger/images/location.svg");
}

int DisassemblerModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_lines.size();
}

int DisassemblerModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 3;
}

QVariant DisassemblerModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_lines.size())
        return QVariant();

    const DisassemblerLine &line = m_lines.at(index.row());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case 0:
            return line.addressDisplay;
        case 1:
            return line.symbolDisplay;
        case 2:
            return line.mnemonic;
        }
    } else if (role == Qt::ToolTipRole) {
        return QString(); 
    } else if (role == Qt::DecorationRole && index.column() == 0) {
        // Return icon that indicates whether this is the active stack frame
        return (index.row() == m_currentLine) ? m_positionIcon : m_emptyIcon;
    }

    return QVariant();
}

QVariant DisassemblerModel::headerData(int section, Qt::Orientation orientation,
    int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        static const char * const headers[] = {
            QT_TR_NOOP("Address"),
            QT_TR_NOOP("Symbol"),
            QT_TR_NOOP("Mnemonic"),
        };
        if (section < 3)
            return tr(headers[section]);
    }
    return QVariant();
}

void DisassemblerModel::setLines(const QList<DisassemblerLine> &lines)
{
    m_lines = lines;
    if (m_currentLine >= m_lines.size())
        m_currentLine = m_lines.size() - 1;
    reset();
}

QList<DisassemblerLine> DisassemblerModel::lines() const
{
    return m_lines;
}



//////////////////////////////////////////////////////////////////
//
// DisassemblerHandler
//
//////////////////////////////////////////////////////////////////

DisassemblerHandler::DisassemblerHandler()
{
    m_model = new DisassemblerModel(this);
}

void DisassemblerHandler::removeAll()
{
    m_model->m_lines.clear();
}

QAbstractItemModel *DisassemblerHandler::model() const
{
    return m_model;
}

void DisassemblerHandler::setLines(const QList<DisassemblerLine> &lines)
{
    m_model->setLines(lines);
}

QList<DisassemblerLine> DisassemblerHandler::lines() const
{
    return m_model->lines();
}

void DisassemblerHandler::setCurrentLine(int line)
{
    m_model->setCurrentLine(line);
}
