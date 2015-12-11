/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Enterprise Qt Quick Profiler Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/

#ifndef FLAMEGRAPHVIEW_H
#define FLAMEGRAPHVIEW_H

#include "flamegraphmodel.h"

#include "qmlprofiler/qmlprofilereventsview.h"
#include <QWidget>
#include <QQuickWidget>

namespace QmlProfilerExtension {
namespace Internal {

class FlameGraphView : public QmlProfiler::QmlProfilerEventsView
{
    Q_OBJECT
public:
    FlameGraphView(QWidget *parent, QmlProfiler::QmlProfilerModelManager *manager);

    void clear() override;
    void restrictToRange(qint64 rangeStart, qint64 rangeEnd) override;
    bool isRestrictedToRange() const override;

public slots:
    void selectByTypeId(int typeIndex) override;
    void onVisibleFeaturesChanged(quint64 features) override;

protected:
    void contextMenuEvent(QContextMenuEvent *ev) override;

private:
    QQuickWidget *m_content;
    FlameGraphModel *m_model;
    bool m_isRestrictedToRange;
};

}
}

#endif // FLAMEGRAPHVIEW_H
