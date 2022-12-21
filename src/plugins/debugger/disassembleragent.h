// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "breakhandler.h"

#include <QObject>

namespace Debugger::Internal {

class DebuggerEngine;
class DisassemblerAgentPrivate;
class DisassemblerLines;
class Location;

class DisassemblerAgent : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString mimeType READ mimeType WRITE setMimeType)
public:
    // Called from Gui
    explicit DisassemblerAgent(DebuggerEngine *engine);
    ~DisassemblerAgent() override;

    void setLocation(const Location &location);
    const Location &location() const;
    void scheduleResetLocation();
    void resetLocation();
    void setContents(const DisassemblerLines &contents);
    void updateLocationMarker();
    void updateBreakpointMarker(const Breakpoint &bp);
    void removeBreakpointMarker(const Breakpoint &bp);

    // Mimetype: "text/a-asm" or some specialized architecture
    QString mimeType() const;
    Q_SLOT void setMimeType(const QString &mt);

    quint64 address() const;
    bool contentsCoversAddress(const QString &contents) const;
    void cleanup();

    // Force reload, e.g. after changing the output flavour.
    void reload();

private:
    void setContentsToDocument(const DisassemblerLines &contents);
    int indexOf(const Location &loc) const;

    DisassemblerAgentPrivate *d;
};

} // Debugger::Internal
