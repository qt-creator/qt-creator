// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <profiler/qmlprofilermodelmanager.h>
#include <profiler/quick3dframemodel.h>

#include <QObject>

namespace Profiler::Internal {

class Quick3DFrameModelTest : public QObject
{
    Q_OBJECT

public:
    Quick3DFrameModelTest();

private slots:
    void testNegativeFrameDelta();

private:
    QmlProfilerModelManager manager;
    Quick3DFrameModel model;
};

} // namespace Profiler::Internal
