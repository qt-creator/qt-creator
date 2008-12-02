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
#ifndef PROFILEREADER_H
#define PROFILEREADER_H

#include "profileevaluator.h"
#include "qtversionmanager.h"

#include <QtCore/QObject>
#include <QtCore/QMap>

namespace Qt4ProjectManager {

namespace Internal {

class ProFileCache;

class ProFileReader : public QObject, public ProFileEvaluator
{
    Q_OBJECT

public:
    ProFileReader(ProFileCache *cache);

    void setQtVersion(QtVersion *qtVersion);
    bool readProFile(const QString &fileName);
    QList<ProFile*> includeFiles() const;

    QString value(const QString &variable) const;

    enum PathValuesMode { AllPaths, ExistingPaths, ExistingFilePaths };
    QStringList absolutePathValues(const QString &variable,
                                   const QString &baseDirectory,
                                   PathValuesMode mode,
                                   const ProFile *pro = 0) const;

signals:
    void errorFound(const QString &error);

private:
    virtual ProFile *parsedProFile(const QString &fileName);
    virtual void releaseParsedProFile(ProFile *proFile);
    virtual void logMessage(const QString &msg);
    virtual void fileMessage(const QString &msg);
    virtual void errorMessage(const QString &msg);

private:
    ProFile *proFileFromCache(const QString &fileName) const;
    ProFileCache *m_cache;
    QMap<QString, ProFile *> m_includeFiles;
};

} //namespace Internal
} //namespace Qt4ProjectManager

#endif // PROFILEREADER_H
