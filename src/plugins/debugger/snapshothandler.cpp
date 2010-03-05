/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "snapshothandler.h"

#include "debuggeractions.h"

#include <utils/qtcassert.h>

#include <QtCore/QAbstractTableModel>
#include <QtCore/QDebug>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>

namespace Debugger {
namespace Internal {

SnapshotData::SnapshotData()
{}

void SnapshotData::clear()
{
    m_frames.clear();
    m_location.clear();
    m_date = QDateTime();
}

QString SnapshotData::function() const
{
    if (m_frames.isEmpty())
        return QString();
    const StackFrame &frame = m_frames.at(0);
    return frame.function + ":" + QString::number(frame.line);
}

QString SnapshotData::toString() const
{
    QString res;
    QTextStream str(&res);
    str << SnapshotHandler::tr("Function:") << ' ' << function() << ' '
        << SnapshotHandler::tr("File:") << ' ' << m_location << ' '
        << SnapshotHandler::tr("Date:") << ' ' << m_date.toString();
    return res;
}

QString SnapshotData::toToolTip() const
{
    QString res;
    QTextStream str(&res);
    str << "<html><body><table>"
        << "<tr><td>" << SnapshotHandler::tr("Function:")
            << "</td><td>" << function() << "</td></tr>"
        << "<tr><td>" << SnapshotHandler::tr("File:")
            << "</td><td>" << QDir::toNativeSeparators(m_location) << "</td></tr>"
        << "</table></body></html>";
    return res;
}

QDebug operator<<(QDebug d, const  SnapshotData &f)
{
    QString res;
    QTextStream str(&res);
    str << f.location();
/*
    str << "level=" << f.level << " address=" << f.address;
    if (!f.function.isEmpty())
        str << ' ' << f.function;
    if (!f.location.isEmpty())
        str << ' ' << f.location << ':' << f.line;
    if (!f.from.isEmpty())
        str << " from=" << f.from;
    if (!f.to.isEmpty())
        str << " to=" << f.to;
*/
    d.nospace() << res;
    return d;
}

////////////////////////////////////////////////////////////////////////
//
// SnapshotHandler
//
////////////////////////////////////////////////////////////////////////

SnapshotHandler::SnapshotHandler(QObject *parent)
  : QAbstractTableModel(parent),
    m_positionIcon(QIcon(":/debugger/images/location_16.png")),
    m_emptyIcon(QIcon(":/debugger/images/empty14.png"))
{
    m_currentIndex = 0;
    connect(theDebuggerAction(OperateByInstruction), SIGNAL(triggered()),
        this, SLOT(resetModel()));
}

SnapshotHandler::~SnapshotHandler()
{
    foreach (const SnapshotData &snapshot, m_snapshots)
        QFile::remove(snapshot.location());
}

int SnapshotHandler::rowCount(const QModelIndex &parent) const
{
    // Since the stack is not a tree, row count is 0 for any valid parent
    return parent.isValid() ? 0 : m_snapshots.size();
}

int SnapshotHandler::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 3;
}

QVariant SnapshotHandler::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_snapshots.size())
        return QVariant();

    if (index.row() == m_snapshots.size()) {
        if (role == Qt::DisplayRole && index.column() == 0)
            return tr("...");
        if (role == Qt::DisplayRole && index.column() == 1)
            return tr("<More>");
        if (role == Qt::DecorationRole && index.column() == 0)
            return m_emptyIcon;
        return QVariant();
    }

    const SnapshotData &snapshot = m_snapshots.at(index.row());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case 0: // Function name of topmost snapshot
            return snapshot.function();
        case 1: // Timestamp
            return snapshot.date().toString();
        case 2: // File name
            return snapshot.location();
        }
        return QVariant();
    }

    if (role == Qt::ToolTipRole) {
        //: Tooltip for variable
        return snapshot.toToolTip();
    }

    if (role == Qt::DecorationRole && index.column() == 0) {
        // Return icon that indicates whether this is the active stack frame
        return (index.row() == m_currentIndex) ? m_positionIcon : m_emptyIcon;
    }

    //if (role == Qt::UserRole)
    //    return QVariant::fromValue(snapshot);

    return QVariant();
}

QVariant SnapshotHandler::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case 0: return tr("Function");
            case 1: return tr("Date");
            case 2: return tr("Location");
        };
    }
    return QVariant();
}

Qt::ItemFlags SnapshotHandler::flags(const QModelIndex &index) const
{
    if (index.row() >= m_snapshots.size())
        return 0;
    if (index.row() == m_snapshots.size())
        return QAbstractTableModel::flags(index);
    //const SnapshotData &snapshot = m_snapshots.at(index.row());
    return true ? QAbstractTableModel::flags(index) : Qt::ItemFlags(0);
}

void SnapshotHandler::removeAll()
{
    m_snapshots.clear();
    m_currentIndex = 0;
    reset();
}

void SnapshotHandler::appendSnapshot(const SnapshotData &snapshot)
{
    m_snapshots.append(snapshot);
    m_currentIndex = m_snapshots.size() - 1;
    reset();
}

void SnapshotHandler::removeSnapshot(int index)
{
    QFile::remove(m_snapshots.at(index).location());
    m_snapshots.removeAt(index);
    if (index == m_currentIndex)
        m_currentIndex = -1;
    else if (index < m_currentIndex)
        --m_currentIndex;
    reset();
}

QList<SnapshotData> SnapshotHandler::snapshots() const
{
    return m_snapshots;
}

SnapshotData SnapshotHandler::setCurrentIndex(int index)
{
    m_currentIndex = index;
    reset();
    return m_snapshots.at(index);
}

} // namespace Internal
} // namespace Debugger
