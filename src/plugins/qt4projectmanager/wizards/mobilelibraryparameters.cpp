/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "mobilelibraryparameters.h"
#include "qtprojectparameters.h"

#include <QTextStream>
#include <QDir>

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
            capabilitySet += QLatin1String(symbianCapability[i].name) + QLatin1Char(' ');
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
    if (libraryType == QtProjectParameters::SharedLibrary) {
        str << "\n"
               "symbian {\n"
               "    MMP_RULES += EXPORTUNFROZEN\n"
               "    TARGET.UID3 = " << symbianUid << "\n"
               "    TARGET.CAPABILITY = " << generateCapabilitySet(symbianCapabilities) << "\n"
               "    TARGET.EPOCALLOWDLLDATA = 1\n"
               "    addFiles.sources = " << fileName << ".dll\n"
               "    addFiles.path = !:/sys/bin\n"
               "    DEPLOYMENT += addFiles\n"
               "}\n";
    } else if (libraryType == QtProjectParameters::Qt4Plugin) {
        str << "\n"
               "symbian {\n"
               "# Load predefined include paths (e.g. QT_PLUGINS_BASE_DIR) to be used in the pro-files\n"
               "    load(data_caging_paths)\n"
               "    MMP_RULES += EXPORTUNFROZEN\n"
               "    TARGET.UID3 = " << symbianUid << "\n"
               "    TARGET.CAPABILITY = " << generateCapabilitySet(symbianCapabilities) << "\n"
               "    TARGET.EPOCALLOWDLLDATA = 1\n"
               "    pluginDeploy.sources = " << fileName << ".dll\n"
               "    pluginDeploy.path = $$QT_PLUGINS_BASE_DIR/" << QDir::fromNativeSeparators(qtPluginDirectory) << "\n"
               "    DEPLOYMENT += pluginDeploy\n"
               "}\n";
    }
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
