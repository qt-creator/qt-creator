/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "snapshothandler.h"

#include "debuggerconstants.h"
#include "debuggerinternalconstants.h"
#include "debuggercore.h"
#include "debuggerengine.h"
#include "debuggerrunner.h"
#include "debuggerstartparameters.h"

#include <utils/qtcassert.h>

#include <QDebug>
#include <QFile>
#include <QIcon>

namespace Debugger {
namespace Internal {

#if 0
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
    return frame.function + QLatin1Char(':') + QString::number(frame.line);
}

QString SnapshotData::toString() const
{
    QString res;
    QTextStream str(&res);
/*    str << SnapshotHandler::tr("Function:") << ' ' << function() << ' '
        << SnapshotHandler::tr("File:") << ' ' << m_location << ' '
        << SnapshotHandler::tr("Date:") << ' ' << m_date.toString(); */
    return res;
}

QString SnapshotData::toToolTip() const
{
    QString res;
    QTextStream str(&res);
    str << "<html><body><table>"
/*
        << "<tr><td>" << SnapshotHandler::tr("Function:")
            << "</td><td>" << function() << "</td></tr>"
        << "<tr><td>" << SnapshotHandler::tr("File:")
            << "</td><td>" << QDir::toNativeSeparators(m_location) << "</td></tr>"
        << "</table></body></html>"; */
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
#endif

////////////////////////////////////////////////////////////////////////
//
// SnapshotHandler
//
////////////////////////////////////////////////////////////////////////

/*!
    \class Debugger::Internal::SnapshotHandler
    \brief A model to represent the snapshots in a QTreeView.

    A snapshot represents a debugging session.
*/

SnapshotHandler::SnapshotHandler()
  : m_positionIcon(QIcon(QLatin1String(":/debugger/images/location_16.png"))),
    m_emptyIcon(QIcon(QLatin1String(":/debugger/images/debugger_empty_14.png")))
{
    m_currentIndex = -1;
}

SnapshotHandler::~SnapshotHandler()
{
    for (int i = m_snapshots.size(); --i >= 0; ) {
        if (DebuggerEngine *engine = at(i)) {
            const DebuggerStartParameters &sp = engine->startParameters();
            if (sp.isSnapshot && !sp.coreFile.isEmpty())
                QFile::remove(sp.coreFile);
        }
    }
}

int SnapshotHandler::rowCount(const QModelIndex &parent) const
{
    // Since the stack is not a tree, row count is 0 for any valid parent
    return parent.isValid() ? 0 : m_snapshots.size();
}

int SnapshotHandler::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 2;
}

QVariant SnapshotHandler::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_snapshots.size())
        return QVariant();

    const DebuggerEngine *engine = at(index.row());

    if (role == SnapshotCapabilityRole)
        return engine && engine->hasCapability(SnapshotCapability);

    if (!engine)
        return QLatin1String("<finished>");

    const DebuggerStartParameters &sp = engine->startParameters();

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case 0:
            return sp.displayName;
        case 1:
            return sp.coreFile.isEmpty() ? sp.executable : sp.coreFile;
        }
        return QVariant();

    case Qt::ToolTipRole:
        return QVariant();

    case Qt::DecorationRole:
        // Return icon that indicates whether this is the active stack frame.
        if (index.column() == 0)
            return (index.row() == m_currentIndex) ? m_positionIcon : m_emptyIcon;
        break;

    default:
        break;
    }
    return QVariant();
}

QVariant SnapshotHandler::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case 0: return tr("Name");
            case 1: return tr("File");
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
    return true ? QAbstractTableModel::flags(index) : Qt::ItemFlags(0);
}

void SnapshotHandler::activateSnapshot(int index)
{
    beginResetModel();
    m_currentIndex = index;
    //qDebug() << "ACTIVATING INDEX: " << m_currentIndex << " OF " << size();
    debuggerCore()->displayDebugger(at(index), true);
    endResetModel();
}

void SnapshotHandler::createSnapshot(int index)
{
    DebuggerEngine *engine = at(index);
    QTC_ASSERT(engine, return);
    engine->createSnapshot();
}

void SnapshotHandler::removeSnapshot(int index)
{
    DebuggerEngine *engine = at(index);
    //qDebug() << "REMOVING " << engine;
    QTC_ASSERT(engine, return);
#if 0
    // See http://sourceware.org/bugzilla/show_bug.cgi?id=11241.
    setState(EngineSetupRequested);
    postCommand("set stack-cache off");
#endif
    //QString fileName = engine->startParameters().coreFile;
    //if (!fileName.isEmpty())
    //    QFile::remove(fileName);
    beginResetModel();
    m_snapshots.removeAt(index);
    if (index == m_currentIndex)
        m_currentIndex = -1;
    else if (index < m_currentIndex)
        --m_currentIndex;
    //engine->quitDebugger();
    endResetModel();
}


void SnapshotHandler::removeAll()
{
    beginResetModel();
    m_snapshots.clear();
    m_currentIndex = -1;
    endResetModel();
}

void SnapshotHandler::appendSnapshot(DebuggerEngine *engine)
{
    beginResetModel();
    m_snapshots.append(engine);
    m_currentIndex = size() - 1;
    endResetModel();
}

void SnapshotHandler::removeSnapshot(DebuggerEngine *engine)
{
    // Could be that the run controls died before it was appended.
    int index = m_snapshots.indexOf(engine);
    if (index != -1)
        removeSnapshot(index);
}

void SnapshotHandler::setCurrentIndex(int index)
{
    beginResetModel();
    m_currentIndex = index;
    endResetModel();
}

DebuggerEngine *SnapshotHandler::at(int i) const
{
    return m_snapshots.at(i).data();
}

} // namespace Internal
} // namespace Debugger
