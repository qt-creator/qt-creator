// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace QmlProfiler {
namespace Internal {

class QmlProfilerBindingLoopsRenderPassTest : public QObject
{
    Q_OBJECT
public:
    explicit QmlProfilerBindingLoopsRenderPassTest(QObject *parent = nullptr);

private slots:
    void testInstance();
    void testUpdate();
};

} // namespace Internal
} // namespace QmlProfiler
