// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../utils/googletest.h"

#include <QObject>

class MockQFileSytemWatcher : public QObject
{
    Q_OBJECT

public:
    MOCK_METHOD1(addPaths, void(const QStringList &));
    MOCK_METHOD1(removePaths, void(const QStringList &));

signals:
    void fileChanged(const QString &);
    void directoryChanged(const QString &);
};
