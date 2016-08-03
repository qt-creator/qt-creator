/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "debuggerconstants.h"

#include <QObject>
#include <QPoint>
#include <QPointer>
#include <QColor>

namespace BinEditor { class EditorService; }

namespace Debugger {
namespace Internal {

class DebuggerEngine;

class MemoryMarkup
{
public:
    MemoryMarkup() {}
    MemoryMarkup(quint64 address, quint64 length, QColor c, const QString &tt)
        : address(address), length(length), color(c), toolTip(tt)
    {}

    quint64 address = 0;
    quint64 length = 0;
    QColor color;
    QString toolTip;
};

class MemoryViewSetupData
{
public:
    MemoryViewSetupData() {}

    quint64 startAddress = 0;
    QString registerName;
    QList<MemoryMarkup> markup;
    QPoint pos;
    QString title;
    bool readOnly = false;        // Read-only.
    bool separateView = false;    // Open a separate view (using the pos-parameter).
    bool trackRegisters = false;  // Address parameter is register number to track
};

class MemoryAgent : public QObject
{
    Q_OBJECT

public:
    MemoryAgent(const MemoryViewSetupData &data, DebuggerEngine *engine);
    ~MemoryAgent();

    void updateContents();
    void addData(quint64 address, const QByteArray &data);
    void setFinished();
    bool isUsable();

    static bool hasBinEditor();

private:
    // The backend, provided by the BinEditor plugin, if loaded.
    BinEditor::EditorService *m_service = nullptr;

    DebuggerEngine *m_engine = nullptr;
    bool m_trackRegisters = false;
};

QList<MemoryMarkup> registerViewMarkup(quint64 address, const QString &regName);
QString registerViewTitle(const QString &registerName, quint64 address = 0);

} // namespace Internal
} // namespace Debugger
