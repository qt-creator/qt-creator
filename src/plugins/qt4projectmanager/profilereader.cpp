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

#include "profilereader.h"

#include <QtCore/QDir>
#include <QtCore/QDebug>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

ProFileReader::ProFileReader() : ProFileEvaluator(&m_option)
{
}

ProFileReader::~ProFileReader()
{
    foreach (ProFile *pf, m_proFiles)
        delete pf;
}

void ProFileReader::setQtVersion(const QtVersion *qtVersion)
{
    if (qtVersion)
        m_option.properties = qtVersion->versionInfo();
    else
        m_option.properties.clear();
}

bool ProFileReader::readProFile(const QString &fileName)
{
    //disable caching -> list of include files is not updated otherwise
    ProFile *pro = new ProFile(fileName);
    if (!queryProFile(pro)) {
        delete pro;
        return false;
    }
    m_includeFiles.insert(fileName, pro);
    m_proFiles.append(pro);
    return accept(pro);
}

ProFile *ProFileReader::parsedProFile(const QString &fileName)
{
    ProFile *pro = ProFileEvaluator::parsedProFile(fileName);
    if (pro) {
        m_includeFiles.insert(fileName, pro);
        m_proFiles.append(pro);
    }
    return pro;
}

void ProFileReader::releaseParsedProFile(ProFile *)
{
    return;
}

QList<ProFile*> ProFileReader::includeFiles() const
{
    QString qmakeMkSpecDir = QFileInfo(propertyValue("QMAKE_MKSPECS")).absoluteFilePath();
    QList<ProFile *> list;
    QMap<QString, ProFile *>::const_iterator it, end;
    end = m_includeFiles.constEnd();
    for (it = m_includeFiles.constBegin(); it != end; ++it) {
        if (!QFileInfo((it.key())).absoluteFilePath().startsWith(qmakeMkSpecDir))
            list.append(it.value());
    }
    return list;
}

QString ProFileReader::value(const QString &variable) const
{
    QStringList vals = values(variable);
    if (!vals.isEmpty())
        return vals.first();

    return QString();
}


void ProFileReader::fileMessage(const QString &message)
{
    Q_UNUSED(message)
    // we ignore these...
}

void ProFileReader::logMessage(const QString &message)
{
    Q_UNUSED(message)
    // we ignore these...
}

void ProFileReader::errorMessage(const QString &message)
{
    emit errorFound(message);
}

ProFile *ProFileReader::proFileFor(const QString &name)
{
    return m_includeFiles.value(name);
}
