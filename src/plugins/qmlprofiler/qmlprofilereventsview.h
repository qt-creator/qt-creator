// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlprofiler_global.h"
#include "qmlprofilermodelmanager.h"

#include <QAbstractItemModel>
#include <QWidget>

namespace QmlProfiler {

class QMLPROFILER_EXPORT QmlProfilerEventsView : public QWidget
{
    Q_OBJECT
public:
    QmlProfilerEventsView(QWidget *parent = nullptr) : QWidget(parent) {}

    virtual void selectByTypeId(int typeIndex) = 0;
    virtual void onVisibleFeaturesChanged(quint64 features) = 0;

signals:
    void gotoSourceLocation(const QString &fileName, int lineNumber, int columnNumber);
    void typeSelected(int typeIndex);
    void showFullRange();
};

} // namespace QmlProfiler
