// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "flamegraphmodel.h"
#include "qmlprofilereventsview.h"

#include <QWidget>
#include <QQuickWidget>

namespace QmlProfiler {
namespace Internal {

class FlameGraphView : public QmlProfilerEventsView
{
    Q_OBJECT
public:
    FlameGraphView(QmlProfilerModelManager *manager, QWidget *parent = nullptr);

    void selectByTypeId(int typeIndex) override;
    void onVisibleFeaturesChanged(quint64 features) override;

protected:
    void contextMenuEvent(QContextMenuEvent *ev) override;

private:
    QQuickWidget *m_content;
    FlameGraphModel *m_model;
};

} // namespace Internal
} // namespace QmlProfiler
