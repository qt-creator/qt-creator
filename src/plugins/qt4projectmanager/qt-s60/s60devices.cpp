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

#include "s60devices.h"
#include "gccetoolchain.h"

#include <projectexplorer/environment.h>

#include <QtCore/QSettings>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QTextStream>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>

namespace {
    const char * const SYMBIAN_SDKS_KEY = "HKEY_LOCAL_MACHINE\\Software\\Symbian\\EPOC SDKs";
    const char * const SYMBIAN_PATH_KEY = "CommonPath";
    const char * const SYMBIAN_DEVICES_FILE = "devices.xml";
    const char * const DEVICES_LIST = "devices";
    const char * const DEVICE = "device";
    const char * const DEVICE_ID = "id";
    const char * const DEVICE_NAME = "name";
    const char * const DEVICE_DEFAULT = "default";
    const char * const DEVICE_EPOCROOT = "epocroot";
    const char * const DEVICE_TOOLSROOT = "toolsroot";
}

namespace Qt4ProjectManager {
namespace Internal {

S60Devices::Device::Device() :
    isDefault(false)
{
}

QString S60Devices::Device::toHtml() const
{
    QString rc;
    QTextStream str(&rc);
    str << "<html><body><table>"
            << "<tr><td><b>" << QCoreApplication::translate("Qt4ProjectManager::Internal::S60Devices::Device", "Id:")
            << "</b></td><td>" << id << "</td></tr>"
            << "<tr><td><b>" << QCoreApplication::translate("Qt4ProjectManager::Internal::S60Devices::Device", "Name:")
            << "</b></td><td>" << name << "</td></tr>"
            << "<tr><td><b>" << QCoreApplication::translate("Qt4ProjectManager::Internal::S60Devices::Device", "EPOC:")
            << "</b></td><td>" << epocRoot << "</td></tr>"
            << "<tr><td><b>" << QCoreApplication::translate("Qt4ProjectManager::Internal::S60Devices::Device", "Tools:")
            << "</b></td><td>" << toolsRoot << "</td></tr>"
            << "<tr><td><b>" << QCoreApplication::translate("Qt4ProjectManager::Internal::S60Devices::Device", "Qt:")
            << "</b></td><td>" << qt << "</td></tr>";
    return rc;
}

S60Devices::S60Devices(QObject *parent)
        : QObject(parent)
{
}

// GNU-Poc stuff
static const char *gnuPocRootC = "GNUPOC_ROOT";

static inline QString msgEnvVarNotSet(const char *var)
{
    return QString::fromLatin1("The environment variable %1 is not set.").arg(QLatin1String(var));
}

static inline QString msgEnvVarDirNotExist(const QString &dir, const char *var)
{
    return QString::fromLatin1("The directory %1 pointed to by the environment variable %2 is not set.").arg(dir, QLatin1String(var));
}

bool S60Devices::readLinux()
{
    // Detect GNUPOC_ROOT/EPOC ROOT
    const QByteArray gnuPocRootA = qgetenv(gnuPocRootC);
    if (gnuPocRootA.isEmpty()) {
        m_errorString = msgEnvVarNotSet(gnuPocRootC);
        return false;
    }

    const QDir gnuPocRootDir(QString::fromLocal8Bit(gnuPocRootA));
    if (!gnuPocRootDir.exists()) {
        m_errorString = msgEnvVarDirNotExist(gnuPocRootDir.absolutePath(), gnuPocRootC);
        return false;
    }

    const QDir epocDir(gnuPocRootDir.absolutePath() + QLatin1String("/symbian-sdks/5.0"));
    if (!epocDir.exists()) {
        m_errorString = QString::fromLatin1("EPOC could not be found at %1.").arg(epocDir.absolutePath());
        return false;
    }
    // Check Qt
    Device device;
    device.id = device.name = QLatin1String("GnuPoc");
    device.epocRoot = epocDir.absolutePath();
    device.toolsRoot = gnuPocRootDir.absolutePath();
    device.isDefault = true;
    m_devices.push_back(device);
    return true;
}

bool S60Devices::read()
{
    m_devices.clear();
    m_errorString.clear();
#ifdef Q_OS_WIN
    return readWin();
#else
    return readLinux();
#endif
}

// Windows EPOC

bool S60Devices::readWin()
{
    // Check the windows registry via QSettings for devices.xml path
    QSettings settings(SYMBIAN_SDKS_KEY, QSettings::NativeFormat);
    QString devicesXmlPath = settings.value(SYMBIAN_PATH_KEY).toString();
    if (devicesXmlPath.isEmpty()) {
        m_errorString = "Could not find installed SDKs in registry.";
        return false;
    }

    devicesXmlPath += QLatin1String("/") + QLatin1String(SYMBIAN_DEVICES_FILE);
    QFile devicesFile(devicesXmlPath);
    if (!devicesFile.open(QIODevice::ReadOnly)) {
        m_errorString = QString("Could not read devices file at %1.").arg(devicesXmlPath);
        return false;
    }
    QXmlStreamReader xml(&devicesFile);
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement() && xml.name() == DEVICES_LIST) {
            if (xml.attributes().value("version") == "1.0") {
                // Look for correct device
                while (!(xml.isEndElement() && xml.name() == DEVICES_LIST) && !xml.atEnd()) {
                    xml.readNext();
                    if (xml.isStartElement() && xml.name() == DEVICE) {
                        Device device;
                        device.id = xml.attributes().value(DEVICE_ID).toString();
                        device.name = xml.attributes().value(DEVICE_NAME).toString();
                        if (xml.attributes().value(DEVICE_DEFAULT).toString() == "yes")
                            device.isDefault = true;
                        else
                            device.isDefault = false;
                        while (!(xml.isEndElement() && xml.name() == DEVICE) && !xml.atEnd()) {
                            xml.readNext();
                            if (xml.isStartElement() && xml.name() == DEVICE_EPOCROOT) {
                                device.epocRoot = xml.readElementText();
                            } else if (xml.isStartElement() && xml.name() == DEVICE_TOOLSROOT) {
                                device.toolsRoot = xml.readElementText();
                            }
                        }
                        if (device.toolsRoot.isEmpty())
                            device.toolsRoot = device.epocRoot;
                        m_devices.append(device);
                    }
                }
            } else {
                xml.raiseError("Invalid 'devices' element version.");
            }
        }
    }
    devicesFile.close();
    if (xml.hasError()) {
        m_errorString = xml.errorString();
        return false;
    }

    return true;
}

bool S60Devices::detectQtForDevices()
{
    for (int i = 0; i < m_devices.size(); ++i) {
        if (!m_devices[i].qt.isEmpty())
            continue;
        QFile qtDll(QString("%1/epoc32/release/winscw/udeb/QtCore.dll").arg(m_devices[i].epocRoot));
        if (!qtDll.exists() || !qtDll.open(QIODevice::ReadOnly)) {
            m_devices[i].qt = QString();
            continue;
        }
        const QString indicator = "\\src\\corelib\\kernel\\qobject.h";
        int indicatorlength = indicator.length();
        QByteArray buffer;
        QByteArray previousBuffer;
        int index = -1;
        while (!qtDll.atEnd()) {
            buffer = qtDll.read(10000);
            index = buffer.indexOf(indicator.toLatin1());
            if (index >= 0)
                break;
            if (!qtDll.atEnd())
                qtDll.seek(qtDll.pos()-indicatorlength);
            previousBuffer = buffer;
        }
        int lastIndex = index;
        while (index >= 0 && buffer.at(index)) --index;
        if (index < 0) { // this is untested
        } else {
            index += 2; // the 0 and another byte for some reason
            m_devices[i].qt = QDir::toNativeSeparators(buffer.mid(index, lastIndex-index));
        }
        qtDll.close();
    }
    emit qtVersionsChanged();
    return true;
}

QString S60Devices::errorString() const
{
    return m_errorString;
}

QList<S60Devices::Device> S60Devices::devices() const
{
    return m_devices;
}

S60Devices::Device S60Devices::deviceForId(const QString &id) const
{
    foreach (const S60Devices::Device &i, m_devices) {
        if (i.id == id) {
            return i;
        }
    }
    return Device();
}

S60Devices::Device S60Devices::deviceForEpocRoot(const QString &root) const
{
    foreach (const S60Devices::Device &i, m_devices) {
        if (i.epocRoot == root) {
            return i;
        }
    }
    return Device();
}

QString S60Devices::cleanedRootPath(const QString &deviceRoot)
{
    QString path = deviceRoot;
    if (path.size() > 1 && path[1] == QChar(':')) {
        path = path.mid(2);
    }

    if (!path.size() || path[path.size()-1] != QChar('\\')) {
        path += QChar('\\');
    }
    return path;
}

// S60ToolChainMixin
S60ToolChainMixin::S60ToolChainMixin(const S60Devices::Device &d) :
    m_device(d)
{
}

const S60Devices::Device & S60ToolChainMixin::device() const
{
    return m_device;
}

bool S60ToolChainMixin::equals(const  S60ToolChainMixin &rhs) const
{
    return m_device.id == rhs.m_device.id
           && m_device.name == rhs.m_device.name;
}

QList<ProjectExplorer::HeaderPath> S60ToolChainMixin::epocHeaderPaths() const
{
    QList<ProjectExplorer::HeaderPath> rc;
    rc << ProjectExplorer::HeaderPath(m_device.epocRoot + QLatin1String("\\epoc32\\include"),
                     ProjectExplorer::HeaderPath::GlobalHeaderPath)
       << ProjectExplorer::HeaderPath(m_device.epocRoot + QLatin1String("\\epoc32\\include\\stdapis"),
                     ProjectExplorer::HeaderPath::GlobalHeaderPath)
       << ProjectExplorer::HeaderPath(m_device.epocRoot + QLatin1String("\\epoc32\\include\\stdapis\\sys"),
                     ProjectExplorer::HeaderPath::GlobalHeaderPath)
       << ProjectExplorer::HeaderPath(m_device.epocRoot + QLatin1String("\\epoc32\\include\\variant"),
                     ProjectExplorer::HeaderPath::GlobalHeaderPath);
    return rc;
}

void S60ToolChainMixin::addEpocToEnvironment(ProjectExplorer::Environment *env) const
{
    env->prependOrSetPath(m_device.epocRoot + QLatin1String("\\epoc32\\tools")); // e.g. make.exe
    env->prependOrSetPath(m_device.epocRoot + QLatin1String("\\epoc32\\gcc\\bin")); // e.g. gcc.exe
    env->prependOrSetPath(m_device.epocRoot + QLatin1String("\\perl\\bin")); // e.g. perl.exe (special SDK version)
    env->set(QLatin1String("EPOCDEVICE"), m_device.id + QLatin1Char(':') + m_device.name);
    env->set(QLatin1String("EPOCROOT"), S60Devices::cleanedRootPath(m_device.epocRoot));
}

QList<ProjectExplorer::HeaderPath> S60ToolChainMixin::gnuPocHeaderPaths() const
{
    return QList<ProjectExplorer::HeaderPath>(); // TODO:
}

void S60ToolChainMixin::addGnuPocToEnvironment(ProjectExplorer::Environment *env) const
{
    env->prependOrSetPath(m_device.toolsRoot + QLatin1String("/bin"));
    //  # trailing "/" is required!
    env->set(QLatin1String("EPOCROOT"),  m_device.epocRoot + QLatin1Char('/'));
}

QDebug operator<<(QDebug db, const S60Devices::Device &d)
{
    QDebug nospace = db.nospace();
    nospace << "id='" << d.id << "' name='" << d.name << "' default="
            << d.isDefault << " Epoc='" << d.epocRoot << "' tools='"
            << d.toolsRoot << "' Qt='" << d.qt << '\'';
    return db;
}

QDebug operator<<(QDebug dbg, const S60Devices &d)
{
    foreach(const S60Devices::Device &device, d.devices())
        dbg << device;
    return dbg;
}

} // namespace Internal
} // namespace Qt4ProjectManager
