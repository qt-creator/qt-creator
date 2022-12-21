// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmljs/qmljsdocument.h>
#include <qmljstools/qmljssemanticinfo.h>

#include <QWaitCondition>
#include <QModelIndex>
#include <QMutex>
#include <QThread>

namespace QmlJSEditor {
namespace Internal {

class SemanticInfoUpdater: public QThread
{
    Q_OBJECT

public:
    SemanticInfoUpdater();
    ~SemanticInfoUpdater() override;

    void abort();
    void update(const QmlJS::Document::Ptr &doc, const QmlJS::Snapshot &snapshot);
    void reupdate(const QmlJS::Snapshot &snapshot);

signals:
    void updated(const QmlJSTools::SemanticInfo &semanticInfo);

protected:
    void run() override;

private:
    QmlJSTools::SemanticInfo makeNewSemanticInfo(const QmlJS::Document::Ptr &doc,
                                                 const QmlJS::Snapshot &snapshot);

private:
    QMutex m_mutex;
    QWaitCondition m_condition;
    bool m_wasCancelled = false;
    QmlJS::Document::Ptr m_sourceDocument;
    QmlJS::Snapshot m_sourceSnapshot;
    QmlJSTools::SemanticInfo m_lastSemanticInfo;
};

} // namespace Internal
} // namespace QmlJSEditor
