/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef MOBILELIBRARYPARAMETERS_H
#define MOBILELIBRARYPARAMETERS_H

#include <QtCore/QString>

QT_BEGIN_NAMESPACE
class QTextStream;
QT_END_NAMESPACE

namespace Qt4ProjectManager {
namespace Internal {

// Additional parameters required for creating mobile
// libraries
struct MobileLibraryParameters {
    enum Type { TypeNone = 0, Symbian = 0x1, Maemo = 0x2 };
    enum Capability {
        CapabilityNone  = 0,
        LocalServices   = 1 << 0,
        Location        = 1 << 1,
        NetworkServices = 1 << 2,
        ReadUserData    = 1 << 3,
        UserEnvironment = 1 << 4,
        WriteUserData   = 1 << 5,
        PowerMgmt       = 1 << 6,
        ProtServ        = 1 << 7,
        ReadDeviceData  = 1 << 8,
        SurroundingsDD  = 1 << 9,
        SwEvent         = 1 << 10,
        TrustedUI       = 1 << 11,
        WriteDeviceData = 1 << 12,
        CommDD          = 1 << 13,
        DiskAdmin       = 1 << 14,
        NetworkControl  = 1 << 15,
        MultimediaDD    = 1 << 16,
        AllFiles        = 1 << 17,
        DRM             = 1 << 18,
        TCB             = 1 << 19
    };

    MobileLibraryParameters();
    void writeProFile(QTextStream &str) const;

private:
    void writeSymbianProFile(QTextStream &str) const;
    void writeMaemoProFile(QTextStream &str) const;

public:
    uint type;
    uint libraryType;
    QString fileName;

    QString symbianUid;
    uint symbianCapabilities;
};

} // namespace Internal
} // namespace Qt4ProjectManager
#endif // MOBILELIBRARYPARAMETERS_H
