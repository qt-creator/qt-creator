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

#ifndef DEBUGGER_DEBUGGERITEMMODEL_H
#define DEBUGGER_DEBUGGERITEMMODEL_H

#include "debuggeritem.h"

#include <QStandardItemModel>
#include <QVariant>

namespace Debugger {
namespace Internal {

// -----------------------------------------------------------------------
// DebuggerItemModel
//------------------------------------------------------------------------

class DebuggerItemModel : public QStandardItemModel
{
    Q_OBJECT

public:
    DebuggerItemModel(QObject *parent);

    QModelIndex currentIndex() const;
    QModelIndex lastIndex() const;
    void setCurrentIndex(const QModelIndex &index);
    QVariant currentDebugger() const { return m_currentDebugger; }
    void addDebugger(const DebuggerItem &item);
    void removeDebugger(const QVariant &id);
    void updateDebugger(const QVariant &id);

public slots:
    void markCurrentDirty();

signals:
    void debuggerAdded(const QVariant &id, const QString &display);
    void debuggerUpdated(const QVariant &id, const QString &display);
    void debuggerRemoved(const QVariant &id);

private:
    // <debug>
    // friend class Debugger::DebuggerKitInformation;
    // friend class DebuggerKitConfigWidget;
    // friend class DebuggerItemConfigWidget;
    // friend class DebuggerOptionsPage;
    // </debug>

    QStandardItem *currentStandardItem() const;
    QStandardItem *findStandardItemById(const QVariant &id) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    QVariant m_currentDebugger;

    QStandardItem *m_autoRoot;
    QStandardItem *m_manualRoot;
    QStringList removed;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_DEBUGGERITEMMODEL_H
