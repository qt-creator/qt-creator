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

#ifndef S60RUNCONFIGBLUETOOTHSTARTER_H
#define S60RUNCONFIGBLUETOOTHSTARTER_H

#include "communicationstarter.h"
#include "bluetoothlistener_gui.h"

namespace Qt4ProjectManager {
namespace Internal {

/* S60RunConfigBluetoothStarter: Creates a listener in 'Listen' mode
 * parented on the Qt Creator core which outputs to the message manager.
 * Provides a static convenience to prompt for both connection types. */

class S60RunConfigBluetoothStarter : public trk::AbstractBluetoothStarter
{
public:
    typedef  trk::AbstractBluetoothStarter::TrkDevicePtr TrkDevicePtr;

    // Convenience function to start communication depending on type,
    // passing on the right messages.
    static trk::PromptStartCommunicationResult
            startCommunication(const TrkDevicePtr &trkDevice,
                               int communicationType,
                               QWidget *msgBoxParent,
                               QString *errorMessage);

protected:
    virtual trk::BluetoothListener *createListener();

private:
    explicit S60RunConfigBluetoothStarter(const TrkDevicePtr& trkDevice, QObject *parent = 0);
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // S60RUNCONFIGBLUETOOTHSTARTER_H
