/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cesdkhandler.h"

#include <utils/environment.h>

#include <QDebug>
#include <QFile>
#include <QXmlStreamReader>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;
using Utils::Environment;

CeSdkInfo::CeSdkInfo()
    : m_major(0), m_minor(0)
{
}

void CeSdkInfo::addToEnvironment(Environment &env)
{
    // qDebug() << "adding " << name() << "to Environment";
    env.set(QLatin1String("INCLUDE"), m_include);
    env.set(QLatin1String("LIB"), m_lib);
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
    if (!vStudioDir.cd(QLatin1String("vcpackages")))
        return false;

    QFile configFile(vStudioDir.absoluteFilePath(QLatin1String("WCE.VCPlatform.config")));
    if (!configFile.exists() || !configFile.open(QIODevice::ReadOnly))
        return false;

    QString currentElement;
    CeSdkInfo currentItem;
    QXmlStreamReader xml(&configFile);
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            currentElement = xml.name().toString();
            if (currentElement == QLatin1String("Platform")) {
                currentItem = CeSdkInfo();
            } else if (currentElement == QLatin1String("Directories")) {
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

CeSdkInfo CeSdkHandler::find(const QString &name) const
{
    typedef QList<CeSdkInfo>::const_iterator Iterator;

    const Iterator cend = m_list.constEnd();
    for (Iterator it = m_list.constBegin(); it != cend; ++it) {
        if (it->name() == name)
            return *it;
    }
    return CeSdkInfo();
}
