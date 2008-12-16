/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "registerhandler.h"

#include "assert.h"
#include "debuggerconstants.h"

#include <QtCore/QAbstractTableModel>
#include <QtCore/QDebug>

#include <QtGui/QColor>

using namespace Debugger;
using namespace Debugger::Internal;
using namespace Debugger::Constants;


//////////////////////////////////////////////////////////////////
//
// RegisterHandler
//
//////////////////////////////////////////////////////////////////

RegisterHandler::RegisterHandler(QObject *parent)
  : QAbstractTableModel(parent)
{
    setProperty(PROPERTY_REGISTER_FORMAT, "x");
}

int RegisterHandler::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_registers.size();
}

int RegisterHandler::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 2;
}

QVariant RegisterHandler::data(const QModelIndex &index, int role) const
{
    static const QVariant red = QColor(200, 0, 0);
    if (!index.isValid() || index.row() >= m_registers.size())
        return QVariant();

    const Register &reg = m_registers.at(index.row());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case 0:
            return reg.name;
        case 1:
            return reg.value;
        }
    }
    if (role == Qt::TextColorRole && reg.changed && index.column() == 1)
        return red;
    return QVariant();
}

QVariant RegisterHandler::headerData(int section, Qt::Orientation orientation,
    int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        static const char * const headers[] = {
            QT_TR_NOOP("Name"),
            QT_TR_NOOP("Value"),
        };
        if (section < 2)
            return tr(headers[section]);
    }
    return QVariant();
}

void RegisterHandler::removeAll()
{
    m_registers.clear();
    reset();
}

bool RegisterHandler::isEmpty() const
{
    return m_registers.isEmpty();
}

void RegisterHandler::setRegisters(const QList<Register> &registers)
{
    m_registers = registers;
    reset();
}

QList<Register> RegisterHandler::registers() const
{
    return m_registers;
}
