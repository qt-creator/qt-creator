// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <QtQml/qqmlregistration.h>

// A trivial C++ type, usable from QML as 'Backend', that provides a
// genuine user-code target for stepping into C++ from QML.
class Backend : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    using QObject::QObject;

    Q_INVOKABLE int process(int value);

private:
    int offset(int value) const;
};
