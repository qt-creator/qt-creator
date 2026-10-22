// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlprofilerstatemanager.h"
#include "qmlprofilermodelmanager.h"

#include <utils/overlaywidget.h>

namespace Profiler::Internal {

class QmlProfilerStateWidget : public Utils::OverlayWidget
{
    Q_OBJECT

public:
    explicit QmlProfilerStateWidget(QmlProfilerStateManager *stateManager,
                                    QmlProfilerModelManager *modelManager,
                                    QWidget *parent = nullptr);
    ~QmlProfilerStateWidget() override;

private:
    void showText(const QString &text);
    void updateDisplay();
    void initialize();
    void clear();

    class QmlProfilerStateWidgetPrivate;
    QmlProfilerStateWidgetPrivate *d;
};

} // namespace Profiler::Internal
