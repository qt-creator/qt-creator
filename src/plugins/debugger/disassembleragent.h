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

#ifndef DEBUGGER_DISASSEMBLERAGENT_H
#define DEBUGGER_DISASSEMBLERAGENT_H

#include <QObject>

namespace Debugger {
namespace Internal {

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
    void updateBreakpointMarkers();

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

#endif // DEBUGGER_DISASSEMBLERAGENT_H
