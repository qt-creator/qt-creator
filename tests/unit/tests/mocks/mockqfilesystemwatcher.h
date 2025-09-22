// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../utils/googletest.h"

#include <QObject>

class MockQFileSytemWatcher : public QObject
{
    Q_OBJECT

public:
    MOCK_METHOD(void, addPaths, (const QStringList &) );
    MOCK_METHOD(void, removePaths, (const QStringList &) );

    void emitDirectoryRemoved(const QString &path) { emit directoryRemoved(path); }

signals:
    void directoryRemoved(const QString &);
    void directoryChanged(const QString &);
};
