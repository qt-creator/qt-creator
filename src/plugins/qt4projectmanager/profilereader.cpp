/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
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
    QString qmakeMkSpecDir = propertyValue("QMAKE_MKSPECS");
    QList<ProFile *> list;
    QMap<QString, ProFile *>::const_iterator it, end;
    end = m_includeFiles.constEnd();
    for (it = m_includeFiles.constBegin(); it != end; ++it) {
        if (!(it.key().startsWith(qmakeMkSpecDir)))
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


// Construct QFileInfo from path value and base directory
// Truncate trailing slashs and make absolute
static inline QFileInfo infoFromPath(QString path,
                              const QString &baseDirectory)
{
    if (path.size() > 1 && path.endsWith(QLatin1Char('/')))
        path.truncate(path.size() - 1);
    QFileInfo info(path);
    if (info.isAbsolute())
        return info;
    return QFileInfo(baseDirectory, path);
}

/*!
  Returns a list of absolute paths
  */
QStringList ProFileReader::absolutePathValues(const QString &variable,
                                              const QString &baseDirectory,
                                              PathValuesMode mode,
                                              const ProFile *pro) const
{
    QStringList rawList;
    if (!pro)
        rawList = values(variable);
    else
        rawList = values(variable, pro);

    if (rawList.isEmpty())
        return QStringList();

    // Normalize list of paths, kill trailing slashes,
    // remove duplicates while maintaining order
    const QChar slash = QLatin1Char('/');
    QStringList result;
    const QStringList::const_iterator rcend = rawList.constEnd();
    for (QStringList::const_iterator it = rawList.constBegin(); it != rcend; ++it) {
        const QFileInfo info = infoFromPath(*it, baseDirectory);
        if (mode == AllPaths || info.exists()) {
            if (mode != ExistingFilePaths || info.isFile()) {
                const QString absPath = info.absoluteFilePath();
                if (!result.contains(absPath))
                    result.push_back(absPath);
            }
        }
    }
    return result;
}

void ProFileReader::fileMessage(const QString &message)
{
    Q_UNUSED(message);
    // we ignore these...
}

void ProFileReader::logMessage(const QString &message)
{
    Q_UNUSED(message);
    // we ignore these...
}

void ProFileReader::errorMessage(const QString &message)
{
    emit errorFound(message);
}

ProFile *ProFileReader::proFileFor(const QString &name)
{
    qDebug()<<"Asking for "<<name;
    qDebug()<<"in "<<m_includeFiles.keys();
    QMap<QString, ProFile *>::const_iterator it = m_includeFiles.constFind(name);
    if (it == m_includeFiles.constEnd())
        return 0;
    else
        return it.value();
}
