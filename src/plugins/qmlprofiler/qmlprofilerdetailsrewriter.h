// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmleventlocation.h"

#include <projectexplorer/runconfiguration.h>
#include <qmljs/qmljsdocument.h>
#include <utils/fileinprojectfinder.h>

#include <QObject>

namespace QmlProfiler {
namespace Internal {

class QmlProfilerDetailsRewriter : public QObject
{
    Q_OBJECT
public:
    explicit QmlProfilerDetailsRewriter(QObject *parent = nullptr);

    void clear();
    void requestDetailsForLocation(int typeId, const QmlEventLocation &location);
    Utils::FilePath getLocalFile(const QString &remoteFile);
    void reloadDocuments();
    void populateFileFinder(const ProjectExplorer::Target *target);

signals:
    void rewriteDetailsString(int typeId, const QString &details);
    void eventDetailsChanged();

private:
    struct PendingEvent {
        QmlEventLocation location;
        int typeId;
    };

    QMultiHash<Utils::FilePath, PendingEvent> m_pendingEvents;
    Utils::FileInProjectFinder m_projectFinder;

    void rewriteDetailsForLocation(const QString &source, QmlJS::Document::Ptr doc, int typeId,
                                   const QmlEventLocation &location);
    void connectQmlModel();
    void disconnectQmlModel();
    void documentReady(QmlJS::Document::Ptr doc);

    friend class QTypeInfo<PendingEvent>;
};

} // namespace Internal
} // namespace QmlProfiler

QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(QmlProfiler::Internal::QmlProfilerDetailsRewriter::PendingEvent, Q_MOVABLE_TYPE);
QT_END_NAMESPACE
