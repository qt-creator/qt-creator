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

#ifndef PROFILEREADER_H
#define PROFILEREADER_H

#include "qtsupport_global.h"

#include <coreplugin/messagemanager.h>
#include <proparser/profileevaluator.h>

#include <QObject>
#include <QMap>
#include <QTimer>

namespace QtSupport {
namespace Internal { class QtSupportPlugin; }

class QTSUPPORT_EXPORT ProMessageHandler : public QObject, public QMakeHandler
{
    Q_OBJECT

public:
    ProMessageHandler(bool verbose = true);
    virtual ~ProMessageHandler() {}

    virtual void aboutToEval(ProFile *, ProFile *, EvalFileType) {}
    virtual void doneWithEval(ProFile *) {}
    virtual void message(int type, const QString &msg, const QString &fileName, int lineNo);
    virtual void fileMessage(const QString &msg);

    void setVerbose(bool on) { m_verbose = on; }

signals:
    void writeMessage(const QString &error, Core::MessageManager::PrintToOutputPaneFlags flag);

private:
    bool m_verbose;
};

class QTSUPPORT_EXPORT ProFileReader : public ProMessageHandler, public QMakeParser, public ProFileEvaluator
{
    Q_OBJECT

public:
    ProFileReader(ProFileGlobals *option, QMakeVfs *vfs);
    ~ProFileReader();

    void setCumulative(bool on);

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
