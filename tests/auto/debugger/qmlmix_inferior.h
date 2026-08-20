// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

// A header of its own, unlike the qmlstack inferior: qmltyperegistrar includes
// the file that declares a QML_ELEMENT type, which duplicates every definition
// that shares it.
class QmlEntryPoint : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    using QObject::QObject;
    Q_INVOKABLE int process(int value);

private:
    int offset(int value) const;
};
