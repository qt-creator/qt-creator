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

#include "sessionengine.h"
#include "debuggeractions.h"
#include "debuggerconstants.h"
#include "debuggerengine.h"
#include "debuggerrunner.h"
#include "debuggerplugin.h"

#include <utils/qtcassert.h>
#include <utils/savedaction.h>

#include <QtCore/QAbstractTableModel>
#include <QtCore/QDebug>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>

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
#endif

////////////////////////////////////////////////////////////////////////
//
// SnapshotHandler
//
////////////////////////////////////////////////////////////////////////

SnapshotHandler::SnapshotHandler()
  : m_positionIcon(QIcon(QLatin1String(":/debugger/images/location_16.png"))),
    m_emptyIcon(QIcon(QLatin1String(":/debugger/images/debugger_empty_14.png")))
{
    m_currentIndex = -1;
}

SnapshotHandler::~SnapshotHandler()
{
    for (int i = m_snapshots.size(); --i >= 0; ) {
        if (DebuggerEngine *engine = engineAt(i)) {
            QString fileName = engine->startParameters().coreFile;
            if (!fileName.isEmpty())
                QFile::remove(fileName);
        }
    }
}

DebuggerEngine *SnapshotHandler::engineAt(int i) const
{
    DebuggerEngine *engine = m_snapshots.at(i)->engine();
    QTC_ASSERT(engine, qDebug() << "ENGINE AT " << i << "DELETED");
    return engine;
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

    const DebuggerEngine *engine = engineAt(index.row());

    if (role == SnapshotCapabilityRole)
        return engine && (engine->debuggerCapabilities() & SnapshotCapability);

    if (!engine)
        return QLatin1String("<finished>");

    const DebuggerStartParameters &sp = engine->startParameters();

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case 0:
            return sp.displayName;
        case 1:
            return sp.coreFile.isEmpty() ? sp.executable : sp.coreFile;
        }
        return QVariant();
    }

    if (role == Qt::ToolTipRole) {
        //: Tooltip for variable
        //return snapshot.toToolTip();
    }

    if (role == Qt::DecorationRole && index.column() == 0) {
        // Return icon that indicates whether this is the active stack frame
        return (index.row() == m_currentIndex) ? m_positionIcon : m_emptyIcon;
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

bool SnapshotHandler::setData
    (const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(value);
    if (index.isValid() && role == RequestCreateSnapshotRole) {
        DebuggerEngine *engine = engineAt(index.row());
        QTC_ASSERT(engine, return false);
        engine->createSnapshot();
        return true;
    }
    if (index.isValid() && role == RequestActivateSnapshotRole) {
        m_currentIndex = index.row();
        //qDebug() << "ACTIVATING INDEX: " << m_currentIndex << " OF " << size();
        DebuggerPlugin::displayDebugger(m_snapshots.at(m_currentIndex));
        reset();
        return true;
    }
    if (index.isValid() && role == RequestRemoveSnapshotRole) {
        DebuggerEngine *engine = engineAt(index.row());
        //qDebug() << "REMOVING " << engine;
        QTC_ASSERT(engine, return false);
        engine->quitDebugger();
        return true;
    }
    return false;
}

#if 0
    // See http://sourceware.org/bugzilla/show_bug.cgi?id=11241.
    setState(EngineSetupRequested);
    postCommand("set stack-cache off");
#endif

void SnapshotHandler::removeAll()
{
    m_snapshots.clear();
    m_currentIndex = -1;
    reset();
}

void SnapshotHandler::appendSnapshot(DebuggerRunControl *rc)
{
    m_snapshots.append(rc);
    m_currentIndex = size() - 1;
    reset();
}

void SnapshotHandler::removeSnapshot(DebuggerRunControl *rc)
{
    // Could be that the run controls died before it was appended.
    int index = m_snapshots.indexOf(rc);
    if (index != -1)
        removeSnapshot(index);
}

void SnapshotHandler::removeSnapshot(int index)
{
    const DebuggerEngine *engine = engineAt(index);
    QTC_ASSERT(engine, return);
    QString fileName = engine->startParameters().coreFile;
    if (!fileName.isEmpty())
        QFile::remove(fileName);
    m_snapshots.removeAt(index);
    if (index == m_currentIndex)
        m_currentIndex = -1;
    else if (index < m_currentIndex)
        --m_currentIndex;
    reset();
}

void SnapshotHandler::setCurrentIndex(int index)
{
    m_currentIndex = index;
    reset();
}

DebuggerRunControl *SnapshotHandler::at(int i) const
{
    return m_snapshots.at(i).data();
}

QList<DebuggerRunControl*> SnapshotHandler::runControls() const
{
    // Return unique list of run controls
    QList<DebuggerRunControl*> rc;
    rc.reserve(m_snapshots.size());
    foreach(const QPointer<DebuggerRunControl> &runControlPtr, m_snapshots)
        if (DebuggerRunControl *runControl = runControlPtr)
            if (!rc.contains(runControl))
                rc.push_back(runControl);
    return rc;
}

} // namespace Internal
} // namespace Debugger
