// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QWidget>

namespace Profiler::Internal {

class QmlProfilerModelManager;
class QmlProfilerDashboardViewPrivate;

class QmlProfilerDashboardView : public QWidget
{
    Q_OBJECT

public:
    explicit QmlProfilerDashboardView(QmlProfilerModelManager *manager,
                                      QWidget *parent = nullptr);

    void updateValues();

private:
    QmlProfilerDashboardViewPrivate *d;
};

} // namespace Profiler::Internal
