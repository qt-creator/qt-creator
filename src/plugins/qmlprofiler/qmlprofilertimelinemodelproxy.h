/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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


#ifndef QMLPROFILERTIMELINEMODELPROXY_H
#define QMLPROFILERTIMELINEMODELPROXY_H

#include "abstracttimelinemodel.h"
#include <qmldebug/qmlprofilereventtypes.h>
#include <qmldebug/qmlprofilereventlocation.h>
#include <QVariantList>
#include "qmlprofilerdatamodel.h"
#include <QColor>

namespace QmlProfiler {
class QmlProfilerModelManager;

namespace Internal {

class RangeTimelineModel : public AbstractTimelineModel
{
    Q_OBJECT
public:

    struct QmlRangeEventStartInstance {
        QmlRangeEventStartInstance(int eventId = -1) :
                eventId(eventId),
                displayRowExpanded(QmlDebug::Constants::QML_MIN_LEVEL),
                displayRowCollapsed(QmlDebug::Constants::QML_MIN_LEVEL),
                bindingLoopHead(-1) {}

        int eventId;

        // not-expanded, per type
        int displayRowExpanded;
        int displayRowCollapsed;
        int bindingLoopHead;
    };

    RangeTimelineModel(QmlDebug::RangeType rangeType, QObject *parent = 0);

    void loadData();
    void clear();

    static QString categoryLabel(QmlDebug::RangeType categoryIndex);
    quint64 features() const;

    int row(int index) const;
    int eventId(int index) const;
    int bindingLoopDest(int index) const;
    QColor color(int index) const;

    QVariantList labels() const;
    QVariantMap details(int index) const;
    QVariantMap location(int index) const;

    int eventIdForTypeIndex(int typeIndex) const;
    int eventIdForLocation(const QString &filename, int line, int column) const;

private:
    class RangeTimelineModelPrivate;
    Q_DECLARE_PRIVATE(RangeTimelineModel)
};

}
}

#endif
