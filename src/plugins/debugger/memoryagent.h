// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "debuggerconstants.h"

#include <QObject>
#include <QPoint>
#include <QPointer>
#include <QColor>

namespace BinEditor { class EditorService; }

namespace Debugger::Internal {

class DebuggerEngine;

class MemoryMarkup
{
public:
    MemoryMarkup() = default;
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
    MemoryViewSetupData() = default;

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
public:
    MemoryAgent(const MemoryViewSetupData &data, DebuggerEngine *engine);
    ~MemoryAgent() override;

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

} // Debugger::Intenal
