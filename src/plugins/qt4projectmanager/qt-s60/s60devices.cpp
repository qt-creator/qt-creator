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
static const char *epocRootC = "EPOCROOT";

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
    const QByteArray epocRootA = qgetenv(epocRootC);
    if (epocRootA.isEmpty()) {
        m_errorString = msgEnvVarNotSet(epocRootC);
        return false;
    }

    const QDir epocRootDir(QString::fromLocal8Bit(epocRootA));
    if (!epocRootDir.exists()) {
        m_errorString = msgEnvVarDirNotExist(epocRootDir.absolutePath(), epocRootC);
        return false;
    }

    // Check Qt
    Device device;
    device.id = device.name = QLatin1String("GnuPoc");
    device.toolsRoot = device.epocRoot = epocRootDir.absolutePath();
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

// Windows: Get list of paths containing common program data
// as pointed to by environment.
static QStringList commonProgramFilesPaths()
{
    const QChar pathSep = QLatin1Char(';');
    QStringList rc;
    const QByteArray commonX86 = qgetenv("CommonProgramFiles(x86)");
    if (!commonX86.isEmpty())
        rc += QString::fromLocal8Bit(commonX86).split(pathSep);
    const QByteArray common = qgetenv("CommonProgramFiles");
    if (!common.isEmpty())
        rc += QString::fromLocal8Bit(common).split(pathSep);
    return rc;
}

// Windows EPOC

// Find the "devices.xml" file containing the SDKs
static QString devicesXmlFile(QString *errorMessage)
{
    const QString devicesFile = QLatin1String(SYMBIAN_DEVICES_FILE);
    // Try registry
    const QSettings settings(QLatin1String(SYMBIAN_SDKS_KEY), QSettings::NativeFormat);
    const QString devicesRegistryXmlPath = settings.value(QLatin1String(SYMBIAN_PATH_KEY)).toString();
    if (!devicesRegistryXmlPath.isEmpty())
        return QDir::cleanPath(devicesRegistryXmlPath + QLatin1Char('/') + devicesFile);
    // Look up common program data files
    const QString symbianDir = QLatin1String("/symbian/");
    foreach(const QString &commonDataDir, commonProgramFilesPaths()) {
        const QFileInfo fi(commonDataDir + symbianDir + devicesFile);
        if (fi.isFile())
            return fi.absoluteFilePath();
    }
    // None found...
    *errorMessage = QString::fromLatin1("The file '%1' containing the device SDK configuration "
                                        "could not be found looking at the registry key "
                                        "%2\\%3 or the common program data directories.").
                                        arg(devicesFile, QLatin1String(SYMBIAN_SDKS_KEY),
                                            QLatin1String(SYMBIAN_PATH_KEY));
    return QString();
}

bool S60Devices::readWin()
{
    const QString devicesXmlPath = devicesXmlFile(&m_errorString);
    if (devicesXmlPath.isEmpty())
        return false;
    QFile devicesFile(devicesXmlPath);
    if (!devicesFile.open(QIODevice::ReadOnly|QIODevice::Text)) {
        m_errorString = QString::fromLatin1("Could not open the devices file %1: %2").arg(devicesXmlPath, devicesFile.errorString());
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
                                device.epocRoot = QDir::fromNativeSeparators(xml.readElementText());
                            } else if (xml.isStartElement() && xml.name() == DEVICE_TOOLSROOT) {
                                device.toolsRoot = QDir::fromNativeSeparators(xml.readElementText());
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
        m_errorString = QString::fromLatin1("Syntax error in devices file %1: %2").
                        arg(devicesXmlPath, xml.errorString());
        return false;
    }
    return true;
}

// Detect a Qt version that is installed into a Symbian SDK
static QString detect_SDK_installedQt(const QString &epocRoot)
{
    const QString coreLibDllFileName = epocRoot + QLatin1String("/epoc32/release/winscw/udeb/QtCore.dll");
    QFile coreLibDllFile(coreLibDllFileName);
    if (!coreLibDllFile.exists() || !coreLibDllFile.open(QIODevice::ReadOnly))
        return false;

    // Do not normalize these backslashes since they are in ARM binaries:
    const QByteArray indicator("\\src\\corelib\\kernel\\qobject.h");
    const int indicatorlength = indicator.size();
    const int chunkSize = 10000;

    int index = -1;
    QByteArray buffer;
    while (true) {
        buffer = coreLibDllFile.read(chunkSize);
        index = buffer.indexOf(indicator);
        if (index >= 0)
            break;
        if (buffer.size() < chunkSize || coreLibDllFile.atEnd())
            return QString();
        coreLibDllFile.seek(coreLibDllFile.pos() - indicatorlength);
    }
    coreLibDllFile.close();

    int lastIndex = index;
    while (index >= 0 && buffer.at(index))
        --index;
    if (index < 0)
        return QString();

    index += 2; // the 0 and another byte for some reason
    return QDir(QString::fromLatin1(buffer.mid(index, lastIndex-index))).absolutePath();
}

// GnuPoc: Detect a Qt version that is symlinked/below an SDK
// TODO: Find a proper way of doing that
static QString detectGnuPocQt(const QString &epocRoot)
{
    const QFileInfo fi(epocRoot + QLatin1String("/qt"));
    if (!fi.exists())
        return QString();
    if (fi.isSymLink())
        return QFileInfo(fi.symLinkTarget()).absoluteFilePath();
    return fi.absoluteFilePath();
}

bool S60Devices::detectQtForDevices()
{
    bool changed = false;
    const int deviceCount = m_devices.size();
    for (int i = 0; i < deviceCount; ++i) {
        Device &device = m_devices[i];
        if (device.qt.isEmpty()) {
            device.qt = detect_SDK_installedQt(device.epocRoot);
            if (device.qt.isEmpty())
                device.qt = detectGnuPocQt(device.epocRoot);
            if (device.qt.isEmpty()) {
                qWarning("Unable to detect Qt version for '%s'.", qPrintable(device.epocRoot));
            } else {
                changed = true;
            }
        }
    }
    if (changed)
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

S60Devices::Device S60Devices::defaultDevice() const
{
    foreach (const S60Devices::Device &i, m_devices) {
        if (i.isDefault) {
            return i;
        }
    }
    return Device();
}

QString S60Devices::cleanedRootPath(const QString &deviceRoot)
{
    QString path = deviceRoot;
#ifdef Q_OS_WIN
    // sbsv2 actually recommends having the DK on a separate drive...
    // But qmake breaks when doing that!
    if (path.size() > 1 && path.at(1) == QChar(':'))
        path = path.mid(2);
#endif

    if (!path.size() || path.at(path.size()-1) != '/')
        path.append('/');
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

    const QString epocRootPath(S60Devices::cleanedRootPath(m_device.epocRoot));

    rc << ProjectExplorer::HeaderPath(epocRootPath,
                     ProjectExplorer::HeaderPath::GlobalHeaderPath)
       << ProjectExplorer::HeaderPath(epocRootPath + QLatin1String("epoc32/include/stdapis"),
                     ProjectExplorer::HeaderPath::GlobalHeaderPath)
       << ProjectExplorer::HeaderPath(epocRootPath + QLatin1String("epoc32/include/stdapis/sys"),
                     ProjectExplorer::HeaderPath::GlobalHeaderPath)
       << ProjectExplorer::HeaderPath(epocRootPath + QLatin1String("epoc32/include/stdapis/stlportv5"),
                     ProjectExplorer::HeaderPath::GlobalHeaderPath)
       << ProjectExplorer::HeaderPath(epocRootPath + QLatin1String("epoc32/include/mw"),
                     ProjectExplorer::HeaderPath::GlobalHeaderPath)
       << ProjectExplorer::HeaderPath(epocRootPath + QLatin1String("epoc32/include/platform/mw"),
                     ProjectExplorer::HeaderPath::GlobalHeaderPath)
       << ProjectExplorer::HeaderPath(epocRootPath + QLatin1String("epoc32/include/platform"),
                     ProjectExplorer::HeaderPath::GlobalHeaderPath)
       << ProjectExplorer::HeaderPath(epocRootPath + QLatin1String("epoc32/include/platform/loc"),
                     ProjectExplorer::HeaderPath::GlobalHeaderPath)
       << ProjectExplorer::HeaderPath(epocRootPath + QLatin1String("epoc32/include/platform/mw/loc"),
                     ProjectExplorer::HeaderPath::GlobalHeaderPath)
       << ProjectExplorer::HeaderPath(epocRootPath + QLatin1String("epoc32/include/platform/loc/sc"),
                     ProjectExplorer::HeaderPath::GlobalHeaderPath)
       << ProjectExplorer::HeaderPath(epocRootPath + QLatin1String("epoc32/include/platform/mw/loc/sc"),
                     ProjectExplorer::HeaderPath::GlobalHeaderPath);
    return rc;
}

void S60ToolChainMixin::addEpocToEnvironment(ProjectExplorer::Environment *env) const
{
    const QString epocRootPath(S60Devices::cleanedRootPath(m_device.epocRoot));

    env->prependOrSetPath(QDir::toNativeSeparators(epocRootPath + QLatin1String("epoc32/tools"))); // e.g. make.exe
    env->prependOrSetPath(QDir::toNativeSeparators(epocRootPath + QLatin1String("epoc32/gcc/bin"))); // e.g. gcc.exe
    env->prependOrSetPath(QDir::toNativeSeparators(epocRootPath + QLatin1String("perl/bin"))); // e.g. perl.exe (special SDK version)

    QString sbsHome(env->value(QLatin1String("SBS_HOME"))); // Do we use Raptor/SBSv2?
    if (!sbsHome.isEmpty())
        env->prependOrSetPath(sbsHome + QDir::separator() + QLatin1String("bin"));
    // No longer set EPOCDEVICE as it conflicts with packaging
    env->set(QLatin1String("EPOCROOT"), QDir::toNativeSeparators(epocRootPath));
}

static const char *gnuPocHeaderPathsC[] = {
    "epoc32/include", "epoc32/include/variant",  "epoc32/include/stdapis",
    "epoc32/include/stdapis/stlport" };

QList<ProjectExplorer::HeaderPath> S60ToolChainMixin::gnuPocHeaderPaths() const
{
    QList<ProjectExplorer::HeaderPath> rc;
    const QString root = m_device.epocRoot + QLatin1Char('/');
    const int count = sizeof(gnuPocHeaderPathsC)/sizeof(const char *);
    for (int i = 0; i < count; i++)
        rc.push_back(ProjectExplorer::HeaderPath(root + QLatin1String(gnuPocHeaderPathsC[i]),
                                                 ProjectExplorer::HeaderPath::GlobalHeaderPath));
    return rc;
}

QStringList S60ToolChainMixin::gnuPocRvctLibPaths(int armver, bool debug) const
{
    QStringList rc;
    QString root;
    QTextStream(&root) << m_device.epocRoot  << "epoc32/release/armv" << armver << '/';
    rc.push_back(root + QLatin1String("lib"));
    rc.push_back(root + (debug ? QLatin1String("udeb") : QLatin1String("urel")));
    return rc;
}

QList<ProjectExplorer::HeaderPath> S60ToolChainMixin::gnuPocRvctHeaderPaths(int major, int minor) const
{
    // Additional header for rvct
    QList<ProjectExplorer::HeaderPath> rc = gnuPocHeaderPaths();
    QString rvctHeader;
    QTextStream(&rvctHeader) << m_device.epocRoot << "/epoc32/include/rvct" << major << '_' << minor;
    rc.push_back(ProjectExplorer::HeaderPath(rvctHeader, ProjectExplorer::HeaderPath::GlobalHeaderPath));
    return rc;
}

void S60ToolChainMixin::addGnuPocToEnvironment(ProjectExplorer::Environment *env) const
{
    env->prependOrSetPath(QDir::toNativeSeparators(m_device.toolsRoot + QLatin1String("/epoc32/tools")));
    const QString epocRootVar = QLatin1String("EPOCROOT");
    // No trailing slash is required here, so, do not perform path cleaning.
    // The variable also should be set since it is currently used for autodetection.
    if (env->find(epocRootVar) == env->constEnd())
        env->set(epocRootVar, m_device.epocRoot);
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
