// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

class QTSUPPORT_EXPORT ProMessageHandler : public QMakeHandler
{
public:
    ProMessageHandler(bool verbose = true, bool exact = true);
    virtual ~ProMessageHandler();

    void aboutToEval(ProFile *, ProFile *, EvalFileType) override {}
    void doneWithEval(ProFile *) override {}
    void message(int type, const QString &msg, const QString &fileName, int lineNo) override;
    void fileMessage(int type, const QString &msg) override;

    void setVerbose(bool on) { m_verbose = on; }
    void setExact(bool on) { m_exact = on; }

private:
    void appendMessage(const QString &msg);

    bool m_verbose;
    bool m_exact;
    QString m_prefix;
    QStringList m_messages;
};

class QTSUPPORT_EXPORT ProFileReader : public ProMessageHandler, public QMakeParser, public ProFileEvaluator
{
public:
    ProFileReader(QMakeGlobals *option, QMakeVfs *vfs);
    ~ProFileReader() override;

    void setCumulative(bool on);

    QHash<ProFile *, QVector<ProFile *> > includeFiles() const;

    void aboutToEval(ProFile *parent, ProFile *proFile, EvalFileType type) override;
    void doneWithEval(ProFile *parent) override;

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
    void discardFiles(const QString &device, const QString &prefix, QMakeVfs *vfs);
    void discardFile(const QString &device, const QString &fileName, QMakeVfs *vfs);
    void incRefCount();
    void decRefCount();

private:
    ProFileCacheManager(QObject *parent);
    ~ProFileCacheManager() override;
    void clear();
    ProFileCache *m_cache = nullptr;
    int m_refCount = 0;
    QTimer m_timer;

    static ProFileCacheManager *s_instance;

    friend class QtSupport::Internal::QtSupportPlugin;
};

} // namespace QtSupport
