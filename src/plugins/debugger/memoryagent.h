/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef DEBUGGER_MEMORYAGENT_H
#define DEBUGGER_MEMORYAGENT_H

#include "debuggerconstants.h"

#include <QObject>
#include <QPoint>
#include <QPointer>
#include <QColor>

namespace Core { class IEditor; }

namespace ProjectExplorer { class Abi; }

namespace Debugger {
namespace Internal {

class DebuggerEngine;
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

class MemoryViewSetupData
{
public:
    MemoryViewSetupData() : parent(0), startAddress(0), flags(0) {}

    QWidget *parent;
    quint64 startAddress;
    QByteArray registerName;
    unsigned flags;
    QList<Internal::MemoryMarkup> markup;
    QPoint pos;
    QString title;
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
    void createBinEditor(const MemoryViewSetupData &data);
    void createBinEditor(quint64 startAddr);
    // Called by engine to create a tooltip.
    void addLazyData(QObject *editorToken, quint64 addr, const QByteArray &data);
    // On stack frame completed and on request.
    void updateContents();
    void closeEditors();
    void closeViews();
    void handleDebuggerFinished();

private slots:
    void fetchLazyData(quint64 block);
    void provideNewRange(quint64 address);
    void handleDataChanged(quint64 address, const QByteArray &data);
    void handleWatchpointRequest(quint64 address, uint size);
    void updateMemoryView(quint64 address, quint64 length);

private:
    void connectBinEditorWidget(QWidget *w);
    bool doCreateBinEditor(const MemoryViewSetupData &data);

    QList<QPointer<Core::IEditor> > m_editors;
    QList<QPointer<MemoryView> > m_views;
    QPointer<DebuggerEngine> m_engine;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_MEMORYAGENT_H
