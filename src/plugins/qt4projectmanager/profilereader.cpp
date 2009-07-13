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
** contact the sales department at http://www.qtsoftware.com/contact.
**
**************************************************************************/

#include "profilereader.h"

#include <QtCore/QDir>
#include <QtCore/QDebug>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

ProFileReader::ProFileReader()
{
}

ProFileReader::~ProFileReader()
{
    foreach (ProFile *pf, m_proFiles)
        delete pf;
}

void ProFileReader::setQtVersion(QtVersion *qtVersion) {
    QHash<QString, QStringList> additionalVariables;
    additionalVariables.insert(QString("QT_BUILD_TREE"), QStringList() << qtVersion->path());
    additionalVariables.insert(QString("QT_SOURCE_TREE"), QStringList() << qtVersion->sourcePath());
    addVariables(additionalVariables);
    addProperties(qtVersion->versionInfo());
    // ### TODO override QT_VERSION property
}

bool ProFileReader::readProFile(const QString &fileName)
{
    //disable caching -> list of include files is not updated otherwise
    QString fn = QFileInfo(fileName).filePath();
    ProFile *pro = new ProFile(fn);
    if (!queryProFile(pro)) {
        delete pro;
        return false;
    }
    m_includeFiles.insert(fn, pro);
    m_proFiles.append(pro);
    return accept(pro);
}

ProFile *ProFileReader::parsedProFile(const QString &fileName)
{
    QString fn =  QFileInfo(fileName).filePath();
    ProFile *pro = ProFileEvaluator::parsedProFile(fn);
    if (pro) {
        m_includeFiles.insert(fn, pro);
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
    QMap<QString, ProFile *>::const_iterator it = m_includeFiles.constFind(name);
    if (it == m_includeFiles.constEnd())
        return 0;
    else
        return it.value();
}
