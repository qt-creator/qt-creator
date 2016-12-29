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

#include "qmlprofilertimelinemodel.h"
#include "qmlprofilereventtypes.h"
#include "qmleventlocation.h"
#include "qmlprofilerconstants.h"

#include <QVariantList>
#include <QColor>
#include <QStack>

namespace QmlProfiler {
class QmlProfilerModelManager;

namespace Internal {

class QmlProfilerRangeModel : public QmlProfilerTimelineModel
{
    Q_OBJECT
public:

    struct QmlRangeEventStartInstance {
        QmlRangeEventStartInstance() :
                displayRowExpanded(1),
                displayRowCollapsed(Constants::QML_MIN_LEVEL),
                bindingLoopHead(-1) {}

        // not-expanded, per type
        int displayRowExpanded;
        int displayRowCollapsed;
        int bindingLoopHead;
    };

    QmlProfilerRangeModel(QmlProfilerModelManager *manager, RangeType range, QObject *parent = 0);

    Q_INVOKABLE int expandedRow(int index) const override;
    Q_INVOKABLE int collapsedRow(int index) const override;
    int bindingLoopDest(int index) const;
    QRgb color(int index) const override;

    QVariantList labels() const override;
    QVariantMap details(int index) const override;
    QVariantMap location(int index) const override;

    int typeId(int index) const override;

    virtual QList<const Timeline::TimelineRenderPass *> supportedRenderPasses() const override;

protected:
    void loadEvent(const QmlEvent &event, const QmlEventType &type) override;
    void finalize() override;
    void clear() override;

private:

    bool supportsBindingLoops() const;
    void computeNestingContracted();
    void computeExpandedLevels();
    void findBindingLoops();

    QVector<QmlRangeEventStartInstance> m_data;
    QStack<int> m_stack;
    QVector<int> m_expandedRowTypes;
};

} // namespace Internal
} // namespace QmlProfiler
