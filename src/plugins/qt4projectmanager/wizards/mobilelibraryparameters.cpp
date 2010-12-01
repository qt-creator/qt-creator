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

#include "mobilelibraryparameters.h"
#include "qtprojectparameters.h"

#include <QtCore/QTextStream>

namespace Qt4ProjectManager {
namespace Internal {

struct SymbianCapability {
    const char *name;
    const int value;
};

static const SymbianCapability symbianCapability[] =
{
    { "LocalServices", MobileLibraryParameters::LocalServices },
    { "Location", MobileLibraryParameters::Location },
    { "NetworkServices", MobileLibraryParameters::NetworkServices },
    { "ReadUserData", MobileLibraryParameters::ReadUserData },
    { "UserEnvironment", MobileLibraryParameters::UserEnvironment },
    { "WriteUserData", MobileLibraryParameters::WriteUserData },
    { "PowerMgmt", MobileLibraryParameters::PowerMgmt },
    { "ProtServ", MobileLibraryParameters::ProtServ },
    { "ReadDeviceData", MobileLibraryParameters::ReadDeviceData },
    { "SurroundingsDD", MobileLibraryParameters::SurroundingsDD },
    { "SwEvent", MobileLibraryParameters::SwEvent },
    { "TrustedUI", MobileLibraryParameters::TrustedUI },
    { "WriteDeviceData", MobileLibraryParameters::WriteDeviceData },
    { "CommDD", MobileLibraryParameters::CommDD },
    { "DiskAdmin", MobileLibraryParameters::DiskAdmin },
    { "NetworkControl", MobileLibraryParameters::NetworkControl },
    { "MultimediaDD", MobileLibraryParameters::MultimediaDD },
    { "AllFiles", MobileLibraryParameters::AllFiles },
    { "DRM", MobileLibraryParameters::DRM },
    { "TCB", MobileLibraryParameters::TCB },

};

QString generateCapabilitySet(uint capabilities)
{
    const int capabilityCount = sizeof(symbianCapability)/sizeof(symbianCapability[0]);
    QString capabilitySet;
    for(int i = 0; i < capabilityCount; ++i)
        if (capabilities&symbianCapability[i].value)
            capabilitySet += QLatin1String(symbianCapability[i].name) + " ";
    return capabilitySet;
}

MobileLibraryParameters::MobileLibraryParameters() :
    type(0), libraryType(TypeNone), symbianCapabilities(CapabilityNone)
{
}

void MobileLibraryParameters::writeProFile(QTextStream &str) const
{
    if (type&Symbian)
        writeSymbianProFile(str);
    if (type&Maemo)
        writeMaemoProFile(str);
}

void MobileLibraryParameters::writeSymbianProFile(QTextStream &str) const
{
    if (libraryType != QtProjectParameters::SharedLibrary)
        return; //nothing to do when the library is not a shared library

    str << "\n"
           "symbian {\n"
           "    MMP_RULES += EXPORTUNFROZEN\n"
           "    TARGET.UID3 = " + symbianUid + "\n"
           "    TARGET.CAPABILITY = " + generateCapabilitySet(symbianCapabilities).toAscii() + "\n"
           "    TARGET.EPOCALLOWDLLDATA = 1\n"
           "    addFiles.sources = " + fileName + ".dll\n"
           "    addFiles.path = !:/sys/bin\n"
           "    DEPLOYMENT += addFiles\n"
           "}\n";
}

void MobileLibraryParameters::writeMaemoProFile(QTextStream &str) const
{
    str << "\n"
           "unix:!symbian {\n"
           "    maemo5 {\n"
           "        target.path = /opt/usr/lib\n"
           "    } else {\n"
           "        target.path = /usr/lib\n"
           "    }\n"
           "    INSTALLS += target\n"
           "}\n";
}

} // namespace Internal
} // namespace Qt4ProjectManager
