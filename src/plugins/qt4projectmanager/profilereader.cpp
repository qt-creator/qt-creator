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
    foreach(ProFile *pf, m_proFiles)
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
//    ProFile *pro = proFileFromCache(fileName);
//    if (!pro) {
//        pro = new ProFile(fileName);
//        if (!queryProFile(pro)) {
//            delete pro;
//            return false;
//        }
//    }
    QString fn =  QFileInfo(fileName).filePath();
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
//    ProFile *pro = proFileFromCache(fileName);
//    if (pro)
//        return pro;
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

ProFile *ProFileReader::proFileFromCache(const QString &fileName) const
{

    QString fn =  QFileInfo(fileName).filePath();
    ProFile *pro = 0;
    if (m_includeFiles.contains(fn))
        pro = m_includeFiles.value(fn);
    return pro;
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
