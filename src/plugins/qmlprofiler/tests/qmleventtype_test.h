// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace QmlProfiler::Internal {

class QmlEventTypeTest : public QObject
{
    Q_OBJECT

public:
    QmlEventTypeTest();

private slots:
    void testAccessors();
    void testFeature();
    void testStreamOps();
};

} // namespace QmlProfiler::Internal
