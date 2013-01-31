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

#ifndef DEBUGGER_MEMORYAGENT_H
#define DEBUGGER_MEMORYAGENT_H

#include "debuggerconstants.h"

#include <QObject>
#include <QPointer>
#include <QString>
#include <QList>
#include <QColor>

QT_FORWARD_DECLARE_CLASS(QPoint)

namespace Core {
class IEditor;
}

namespace ProjectExplorer {
class Abi;
}

namespace Debugger {

class DebuggerEngine;

namespace Internal {
class MemoryView;
class MemoryMarkup
{
public:
    MemoryMarkup(quint64 a = 0, quint64 l = 0, QColor c = Qt::yellow,
                 const QString &tt = QString()) :
        address(a), length(l), color(c), toolTip(tt) {}

    quint64 address;
    quint64 length;
    QColor color;
    QString toolTip;
};

class MemoryAgent : public QObject
{
    Q_OBJECT

public:
    explicit MemoryAgent(DebuggerEngine *engine);
    ~MemoryAgent();

    enum { BinBlockSize = 1024 };
    enum { DataRange = 1024 * 1024 };

    bool hasVisibleEditor() const;

    static bool isBigEndian(const ProjectExplorer::Abi &a);
    static quint64 readInferiorPointerValue(const unsigned char *data, const ProjectExplorer::Abi &a);

public slots:
    // Called by engine to create a new view.
    void createBinEditor(quint64 startAddr, unsigned flags,
                         const QList<MemoryMarkup> &ml, const QPoint &pos,
                         const QString &title, QWidget *parent);
    void createBinEditor(quint64 startAddr);
    // Called by engine to create a tooltip.
    void addLazyData(QObject *editorToken, quint64 addr, const QByteArray &data);
    // On stack frame completed and on request.
    void updateContents();
    void closeEditors();
    void closeViews();

private slots:
    void fetchLazyData(Core::IEditor *, quint64 block);
    void provideNewRange(Core::IEditor *editor, quint64 address);
    void handleDataChanged(Core::IEditor *editor, quint64 address,
        const QByteArray &data);
    void handleWatchpointRequest(quint64 address, uint size);
    void updateMemoryView(quint64 address, quint64 length);
    void engineStateChanged(Debugger::DebuggerState s);

private:
    void connectBinEditorWidget(QWidget *w);
    bool doCreateBinEditor(quint64 startAddr, unsigned flags,
                           const QList<MemoryMarkup> &ml, const QPoint &pos,
                           QString title, QWidget *parent);

    QList<QPointer<Core::IEditor> > m_editors;
    QList<QPointer<MemoryView> > m_views;
    QPointer<DebuggerEngine> m_engine;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_MEMORYAGENT_H
