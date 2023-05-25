// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "../utils/googletest.h"

#include <QObject>

class MockTimer : public QObject
{
    Q_OBJECT

public:
    MockTimer();
    ~MockTimer();

    MOCK_METHOD1(start, void(int));

    void setSingleShot(bool);

    void emitTimoutIfStarted();

signals:
    void timeout();

private:
    bool m_isStarted = false;
};
