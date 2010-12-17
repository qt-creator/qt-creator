/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef DEBUGGER_DISASSEMBLERAGENT_H
#define DEBUGGER_DISASSEMBLERAGENT_H

#include "disassemblerlines.h"

#include <QtCore/QObject>
#include <QtCore/QPointer>

namespace Core {
class IEditor;
}

namespace Debugger {

class DebuggerEngine;

namespace Internal {

class Location;
class DisassemblerAgentPrivate;

class DisassemblerAgent : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString mimeType READ mimeType WRITE setMimeType)
public:
    // Called from Gui
    explicit DisassemblerAgent(DebuggerEngine *engine);
    ~DisassemblerAgent();

    void setTryMixed(bool on);
    void setLocation(const Location &location);
    const Location &location() const;
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
    bool isMixed() const;

    // Return address of an assembly line "0x0dfd  bla"
    static quint64 addressFromDisassemblyLine(const QString &data);

private:
    DisassemblerAgentPrivate *d;
};


} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_DISASSEMBLERAGENT_H
