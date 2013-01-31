/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef PROFILEREADER_H
#define PROFILEREADER_H

#include "qtsupport_global.h"
#include "proparser/qmakeglobals.h"
#include "proparser/qmakeparser.h"
#include "proparser/qmakeevaluator.h"
#include "proparser/profileevaluator.h"

#include <QObject>
#include <QMap>
#include <QTimer>

namespace QtSupport {
namespace Internal {
class QtSupportPlugin;
}

class QTSUPPORT_EXPORT ProMessageHandler : public QObject, public QMakeHandler
{
    Q_OBJECT

public:
    ProMessageHandler(bool verbose = false);
    virtual ~ProMessageHandler() {}

    virtual void aboutToEval(ProFile *, ProFile *, EvalFileType) {}
    virtual void doneWithEval(ProFile *) {}
    virtual void message(int type, const QString &msg, const QString &fileName, int lineNo);
    virtual void fileMessage(const QString &msg);

signals:
    void errorFound(const QString &error);

private:
    bool m_verbose;
};

class QTSUPPORT_EXPORT ProFileReader : public ProMessageHandler, public QMakeParser, public ProFileEvaluator
{
    Q_OBJECT

public:
    ProFileReader(ProFileGlobals *option);
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

class QTSUPPORT_EXPORT ProFileCacheManager : public QObject
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

    friend class QtSupport::Internal::QtSupportPlugin;
};

} // namespace QtSupport

#endif // PROFILEREADER_H
