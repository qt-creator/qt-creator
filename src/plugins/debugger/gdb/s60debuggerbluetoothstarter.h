/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef S60DEBUGGERBLUETOOTHSTARTER_H
#define S60DEBUGGERBLUETOOTHSTARTER_H

#include "communicationstarter.h"
#include "bluetoothlistener_gui.h"

namespace Debugger {
namespace Internal {

/* S60DebuggerBluetoothStarter: Creates a listener in 'Listen' mode
 * parented on the Debugger manager which outputs to the debugger window.
 * Note: This is a "last resort" starter, normally, the run configuration
 * should have already started a listener.
 * Provides a static convenience to prompt for both connection types.  */

class S60DebuggerBluetoothStarter : public trk::AbstractBluetoothStarter
{
public:
    static trk::PromptStartCommunicationResult
        startCommunication(const TrkDevicePtr &trkDevice,
                           QWidget *msgBoxParent,
                           QString *errorMessage);

protected:
    virtual trk::BluetoothListener *createListener();

private:
    explicit S60DebuggerBluetoothStarter(const TrkDevicePtr& trkDevice, QObject *parent = 0);
};

} // namespace Internal
} // namespace Debugger

#endif // S60DEBUGGERBLUETOOTHSTARTER_H
