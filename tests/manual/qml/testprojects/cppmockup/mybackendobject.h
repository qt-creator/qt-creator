
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

class MyBackendObject : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool test READ test write setTest)

public:
    explicit MyBackendObject(QObject *parent = 0);

    bool test() const;
    void setTest(bool b);

public slots:
};
