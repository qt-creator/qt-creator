// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlprofilerstatemanager.h"
#include "qmlprofilermodelmanager.h"

#include <QFrame>

namespace QmlProfiler {
namespace Internal {

class QmlProfilerStateWidget : public QFrame
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
    void reposition();
    void initialize();
    void clear();

    class QmlProfilerStateWidgetPrivate;
    QmlProfilerStateWidgetPrivate *d;
};

} // namespace Internal
} // namespace QmlProfiler
