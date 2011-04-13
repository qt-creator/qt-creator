/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef DEBUGGER_MEMORYAGENT_H
#define DEBUGGER_MEMORYAGENT_H

#include <QtCore/QObject>
#include <QtCore/QPointer>

namespace Core {
class IEditor;
}

namespace Debugger {

class DebuggerEngine;

namespace Internal {

class MemoryAgent : public QObject
{
    Q_OBJECT

public:
    explicit MemoryAgent(DebuggerEngine *engine);
    ~MemoryAgent();

    enum { BinBlockSize = 1024 };
    bool hasVisibleEditor() const;

public slots:
    // Called by engine to create a new view.
    void createBinEditor(quint64 startAddr);
    // Called by engine to trigger update of contents.
    void updateContents();
    // Called by engine to pass updated contents.
    void addLazyData(QObject *editorToken, quint64 addr, const QByteArray &data);

private slots:
    void fetchLazyData(Core::IEditor *, quint64 block);
    void provideNewRange(Core::IEditor *editor, quint64 address);
    void handleStartOfFileRequested(Core::IEditor *editor);
    void handleEndOfFileRequested(Core::IEditor *editor);
    void handleDataChanged(Core::IEditor *editor, quint64 address,
        const QByteArray &data);

private:
    QList<QPointer<Core::IEditor> > m_editors;
    QPointer<DebuggerEngine> m_engine;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_MEMORYAGENT_H
