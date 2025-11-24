// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

class QmlEventLocationTest : public QObject
{
    Q_OBJECT

public:
    QmlEventLocationTest();

private slots:
    void testCtor();
    void testStreamOps();
};
