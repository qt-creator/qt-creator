// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "flamegraphmodel.h"
#include "qmlprofilereventsview.h"

#include <tracing/flamegraphwidget.h>

#include <QWidget>

namespace Profiler::Internal {

class FlameGraphView : public QmlProfilerEventsView
{
    Q_OBJECT
public:
    FlameGraphView(QmlProfilerModelManager *manager, QWidget *parent = nullptr);

    void selectByTypeId(int typeIndex) override;
    void onVisibleFeaturesChanged(quint64 features) override;

signals:
    void detailsChanged(const QString &title, const QList<QPair<QString, QString>> &content);
    void detailsCleared();

protected:
    void contextMenuEvent(QContextMenuEvent *ev) override;

private:
    Timeline::FlameGraphWidget *m_content;
    FlameGraphModel *m_model;
};

} // namespace Profiler::Internal
