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
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
** 
***************************************************************************/
#ifndef DEBUGGER_REGISTERHANDLER_H
#define DEBUGGER_REGISTERHANDLER_H

#include <QtCore/QAbstractTableModel>

namespace Debugger {
namespace Internal {

class Register
{
public:
    Register() : changed(true) {}
    Register(QString const &name_) : name(name_), changed(true) {}

public:
    QString name;
    QString value;
    bool changed;
};

class RegisterHandler : public QAbstractTableModel
{
    Q_OBJECT

public:
    RegisterHandler(QObject *parent = 0);

    void sessionClosed();
    QAbstractItemModel *model() { return this; }

    bool isEmpty() const; // nothing known so far?
    void setRegisters(const QList<Register> &registers);
    QList<Register> registers() const;
    void removeAll();

private:
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const;

    QList<Register> m_registers;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_REGISTERHANDLER_H
