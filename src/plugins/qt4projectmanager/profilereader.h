/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef PROFILEREADER_H
#define PROFILEREADER_H

#include "profileparser.h"
#include "profileevaluator.h"

#include <QtCore/QObject>
#include <QtCore/QMap>
#include <QtCore/QTimer>

namespace Qt4ProjectManager {
namespace Internal {

class ProMessageHandler : public QObject,
                          public ProFileParserHandler, public ProFileEvaluatorHandler
{
    Q_OBJECT

public:
    ProMessageHandler(bool verbose = false);
    virtual ~ProMessageHandler() {}

    virtual void aboutToEval(ProFile *, ProFile *, EvalFileType) {}
    virtual void doneWithEval(ProFile *) {}
    virtual void parseError(const QString &filename, int lineNo, const QString &msg);
    virtual void configError(const QString &msg);
    virtual void evalError(const QString &filename, int lineNo, const QString &msg);
    virtual void fileMessage(const QString &msg);

signals:
    void errorFound(const QString &error);

private:
    bool m_verbose;
};

class ProFileReader : public ProMessageHandler, public ProFileParser, public ProFileEvaluator
{
    Q_OBJECT

public:
    ProFileReader(ProFileOption *option);
    ~ProFileReader();

    QList<ProFile*> includeFiles() const;

    ProFile *proFileFor(const QString &name);

    virtual void aboutToEval(ProFile *parent, ProFile *proFile, EvalFileType type);
    virtual void doneWithEval(ProFile *parent);

private:
    QMap<QString, ProFile *> m_includeFiles;
    QList<ProFile *> m_proFiles;
    int m_ignoreLevel;
};

class ProFileCacheManager : public QObject
{
    Q_OBJECT

public:
    static ProFileCacheManager *instance() { return s_instance; }
    ProFileCache *cache();
    void discardFiles(const QString &prefix);
    void discardFile(const QString &fileName);
    void incRefCount();
    void decRefCount();

private:
    ProFileCacheManager(QObject *parent);
    ~ProFileCacheManager();
    Q_SLOT void clear();
    ProFileCache *m_cache;
    int m_refCount;
    QTimer m_timer;

    static ProFileCacheManager *s_instance;

    friend class Qt4ProjectManagerPlugin;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // PROFILEREADER_H
