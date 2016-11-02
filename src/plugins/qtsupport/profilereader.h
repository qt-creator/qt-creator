/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "qtsupport_global.h"

#include <coreplugin/messagemanager.h>
#include <proparser/profileevaluator.h>

#include <QObject>
#include <QMap>
#include <QVector>
#include <QTimer>

namespace QtSupport {
namespace Internal { class QtSupportPlugin; }

class QTSUPPORT_EXPORT ProMessageHandler : public QObject, public QMakeHandler
{
    Q_OBJECT

public:
    ProMessageHandler(bool verbose = true, bool exact = true);
    virtual ~ProMessageHandler() {}

    virtual void aboutToEval(ProFile *, ProFile *, EvalFileType) {}
    virtual void doneWithEval(ProFile *) {}
    virtual void message(int type, const QString &msg, const QString &fileName, int lineNo);
    virtual void fileMessage(int type, const QString &msg);

    void setVerbose(bool on) { m_verbose = on; }
    void setExact(bool on) { m_exact = on; }

signals:
    void writeMessage(const QString &error, Core::MessageManager::PrintToOutputPaneFlags flag);

private:
    bool m_verbose;
    bool m_exact;
    QString m_prefix;
};

class QTSUPPORT_EXPORT ProFileReader : public ProMessageHandler, public QMakeParser, public ProFileEvaluator
{
    Q_OBJECT

public:
    ProFileReader(QMakeGlobals *option, QMakeVfs *vfs);
    ~ProFileReader();

    void setCumulative(bool on);

    QHash<ProFile *, QVector<ProFile *> > includeFiles() const;

    virtual void aboutToEval(ProFile *parent, ProFile *proFile, EvalFileType type);
    virtual void doneWithEval(ProFile *parent);

private:
    // Tree of ProFiles, mapping from parent to children
    QHash<ProFile *, QVector<ProFile *> > m_includeFiles;
    // One entry per ProFile::ref() call, might contain duplicates
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
    void clear();
    ProFileCache *m_cache;
    int m_refCount;
    QTimer m_timer;

    static ProFileCacheManager *s_instance;

    friend class QtSupport::Internal::QtSupportPlugin;
};

} // namespace QtSupport
