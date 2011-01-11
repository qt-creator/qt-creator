/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "cesdkhandler.h"

#include <utils/environment.h>

#include <QtCore/QFile>
#include <QtCore/QDebug>
#include <QtCore/QXmlStreamReader>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;
using Utils::Environment;

CeSdkInfo::CeSdkInfo()
    : m_major(0), m_minor(0)
{
}

void CeSdkInfo::addToEnvironment(Environment &env)
{
    qDebug() << "adding " << name() << "to Environment";
    env.set("INCLUDE", m_include);
    env.set("LIB", m_lib);
    env.prependOrSetPath(m_bin);
}

CeSdkHandler::CeSdkHandler()
{
}

bool CeSdkHandler::parse(const QString &vsdir)
{
    // look at the file at %VCInstallDir%/vcpackages/WCE.VCPlatform.config
    // and scan through all installed sdks...
    m_list.clear();

    VCInstallDir = vsdir;

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
        qWarning("CeSdkHandler::parse(): XML ERROR: %d : %s", int(xml.lineNumber()), qPrintable(xml.errorString()));
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
