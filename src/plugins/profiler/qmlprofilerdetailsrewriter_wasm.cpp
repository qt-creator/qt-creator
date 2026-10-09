// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilerdetailsrewriter.h"

/*!
    The WebAssembly implementation of \l{QmlProfilerDetailsRewriter}.

    The rewriter turns an event's location into a better description by parsing
    the QML it came from. That needs two things this build does not have: the
    QML parser, and the source files themselves. populateFileFinder() is already
    a no-op here because it goes through QtSupport, so the finder never learns
    where a project is and getLocalFile() could never resolve anything anyway.

    Rather than leave the parser linked in for a class that cannot act, keep the
    interface and do nothing: events keep the details the trace itself carries.
    That is what the desktop build shows too when no sources can be found.
*/

namespace Profiler::Internal {

QmlProfilerDetailsRewriter::QmlProfilerDetailsRewriter(QObject *parent)
    : QObject(parent)
{}

void QmlProfilerDetailsRewriter::clear()
{
    m_pendingEvents.clear();
}

void QmlProfilerDetailsRewriter::requestDetailsForLocation(
    int typeId, const QmlDebug::QmlEventLocation &location)
{
    Q_UNUSED(typeId)
    Q_UNUSED(location)
}

Utils::FilePath QmlProfilerDetailsRewriter::getLocalFile(const QString &remoteFile)
{
    // No project to find it in, and no parser to tell a QML file from anything
    // else: the same empty answer the desktop build gives for an unknown file.
    Q_UNUSED(remoteFile)
    return {};
}

void QmlProfilerDetailsRewriter::reloadDocuments()
{
    emit eventDetailsChanged();
}

void QmlProfilerDetailsRewriter::populateFileFinder(const ProjectExplorer::BuildConfiguration *bc)
{
    Q_UNUSED(bc)
}

} // namespace Profiler::Internal
