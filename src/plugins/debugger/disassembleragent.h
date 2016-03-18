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

#include <QObject>

namespace Debugger {
namespace Internal {

class Breakpoint;
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
    ~DisassemblerAgent();

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

} // namespace Internal
} // namespace Debugger
