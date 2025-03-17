// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmlprofiler/qmlevent.h>

#include <QObject>

namespace QmlProfiler::Internal {

class QmlEventTest : public QObject
{
    Q_OBJECT

public:
    QmlEventTest();

private slots:
    void testCtors();
    void testNumbers();
    void testMaxSize();
    void testStreamOps();

};

} // namespace QmlProfiler::Internal

