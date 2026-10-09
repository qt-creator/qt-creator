// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldebug/qmleventlocation.h>
#include <projectexplorer/runconfiguration.h>
#ifndef QTPROFILER_WASM
#include <qmljs/qmljsdocument.h>
#endif
#include <utils/fileinprojectfinder.h>

#include <QObject>

namespace Profiler::Internal {

class QmlProfilerDetailsRewriter : public QObject
{
    Q_OBJECT

public:
    explicit QmlProfilerDetailsRewriter(QObject *parent = nullptr);

    void clear();
    void requestDetailsForLocation(int typeId, const QmlDebug::QmlEventLocation &location);
    Utils::FilePath getLocalFile(const QString &remoteFile);
    void reloadDocuments();
    void populateFileFinder(const ProjectExplorer::BuildConfiguration *bc);

signals:
    void rewriteDetailsString(int typeId, const QString &details);
    void eventDetailsChanged();

private:
    struct PendingEvent {
        QmlDebug::QmlEventLocation location;
        int typeId;
    };

    QMultiHash<Utils::FilePath, PendingEvent> m_pendingEvents;
    Utils::FileInProjectFinder m_projectFinder;

#ifndef QTPROFILER_WASM
    // Resolving an event to its place in the source needs the QML parser, which
    // the standalone viewer leaves out; see qmlprofilerdetailsrewriter_wasm.cpp.
    void rewriteDetailsForLocation(const QString &source, QmlJS::Document::Ptr doc, int typeId,
                                   const QmlDebug::QmlEventLocation &location);
    void connectQmlModel();
    void disconnectQmlModel();
    void documentReady(QmlJS::Document::Ptr doc);
#endif

    friend class QTypeInfo<PendingEvent>;
};

} // namespace Profiler::Internal

QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(Profiler::Internal::QmlProfilerDetailsRewriter::PendingEvent, Q_MOVABLE_TYPE);
QT_END_NAMESPACE
