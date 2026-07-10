// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

class QmlEntryPoint : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;
    Q_INVOKABLE int process(int value);
};
