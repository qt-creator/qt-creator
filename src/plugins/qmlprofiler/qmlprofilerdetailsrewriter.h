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
    QString getLocalFile(const QString &remoteFile);
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

    QMultiHash<QString, PendingEvent> m_pendingEvents;
    Utils::FileInProjectFinder m_projectFinder;
    QHash<QString, QString> m_filesCache;

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
