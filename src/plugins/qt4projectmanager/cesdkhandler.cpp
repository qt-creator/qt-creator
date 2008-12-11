/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "cesdkhandler.h"

#include <QtCore/QFile>
#include <QtCore/QDebug>
#include <QtCore/QXmlStreamReader>

using namespace Qt4ProjectManager::Internal;
using ProjectExplorer::Environment;

CeSdkInfo::CeSdkInfo()
    : m_major(0), m_minor(0)
{
}

Environment CeSdkInfo::addToEnvironment(const Environment &env)
{
    qDebug() << "adding " << name() << "to Environment";
    Environment e(env);
    e.set("INCLUDE", m_include);
    e.set("LIB", m_lib);
    e.prependOrSetPath(m_bin);
    qDebug()<<e.toStringList();
    return e;
}

CeSdkHandler::CeSdkHandler()
{
}

QString CeSdkHandler::platformName(const QString &qtpath)
{
    QString platformName;
    QString CE_SDK;
    QString CE_ARCH;
    QFile f(qtpath);
    if (f.exists() && f.open(QIODevice::ReadOnly)) {
        while (!f.atEnd()) {
            QByteArray line = f.readLine();
            if (line.startsWith("CE_SDK")) {
                int index = line.indexOf('=');
                if (index >= 0) {
                    CE_SDK = line.mid(index + 1).trimmed();
                }
            } else if (line.startsWith("CE_ARCH")) {
                int index = line.indexOf('=');
                if (index >= 0) {
                    CE_ARCH = line.mid(index + 1).trimmed();
                }
            }
            if (!CE_SDK.isEmpty() && !CE_ARCH.isEmpty()) {
                platformName = CE_SDK + " (" + CE_ARCH + ")";
                break;
            }
        }
    }
    return platformName;
}

bool CeSdkHandler::parse(const QString &vsdir)
{
    // look at the file at %VCInstallDir%/vcpackages/WCE.VCPlatform.config
    // and scan through all installed sdks...    
    m_list.clear();

    VCInstallDir = vsdir + "/VC/";
    VSInstallDir = vsdir;

    QDir vStudioDir(VCInstallDir);
    if (!vStudioDir.cd("vcpackages"))
        return false;

    QFile configFile(vStudioDir.absoluteFilePath(QLatin1String("WCE.VCPlatform.config")));
    qDebug()<<"##";
    if (!configFile.exists() || !configFile.open(QIODevice::ReadOnly))
        return false;

    qDebug()<<"parsing";
    
    QString currentElement;
    CeSdkInfo currentItem;
    QXmlStreamReader xml(&configFile);
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            currentElement = xml.name().toString();
            if (currentElement == QLatin1String("Platform"))
                currentItem = CeSdkInfo();
            else if (currentElement == QLatin1String("Directories")) {
                QXmlStreamAttributes attr = xml.attributes();
                currentItem.m_include = fixPaths(attr.value(QLatin1String("Include")).toString());
                currentItem.m_lib = fixPaths(attr.value(QLatin1String("Library")).toString());
                currentItem.m_bin = fixPaths(attr.value(QLatin1String("Path")).toString());
            }
        } else if (xml.isEndElement()) {
            if (xml.name().toString() == QLatin1String("Platform"))
                m_list.append(currentItem);
        } else if (xml.isCharacters() && !xml.isWhitespace()) {
            if (currentElement == QLatin1String("PlatformName"))
                currentItem.m_name = xml.text().toString();
            else if (currentElement == QLatin1String("OSMajorVersion"))
                currentItem.m_major = xml.text().toString().toInt();
            else if (currentElement == QLatin1String("OSMinorVersion"))
                currentItem.m_minor = xml.text().toString().toInt();
        }
    }

    if (xml.error() && xml.error() != QXmlStreamReader::PrematureEndOfDocumentError) {
        qWarning() << "XML ERROR:" << xml.lineNumber() << ": " << xml.errorString();
        return false;
    }
    return m_list.size() > 0 ? true : false;
}

CeSdkInfo CeSdkHandler::find(const QString &name)
{
    qDebug() << "looking for platform " << name;
    for (QList<CeSdkInfo>::iterator it = m_list.begin(); it != m_list.end(); ++it) {
        qDebug() << "...." << it->name();
        if (it->name() == name)
            return *it;
    }
    return CeSdkInfo();
}
