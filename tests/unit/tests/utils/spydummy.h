// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

class SpyDummy : public QObject
{
    Q_OBJECT
public:
    explicit SpyDummy(QObject *parent = 0);
    ~SpyDummy();

signals:
    void tableIsReady();
    void databaseIsOpened();
    void databaseIsClosed();
};
