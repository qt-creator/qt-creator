// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../qmldesignerbase_global.h"

#include <QtQml/qqml.h>

#include <QLocale>

namespace QmlDesigner {

class QMLDESIGNERBASE_EXPORT StudioQuickUtils : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QLocale locale READ locale NOTIFY localeChanged)

public:
    ~StudioQuickUtils();

    StudioQuickUtils(const StudioQuickUtils &) = delete;
    void operator=(const StudioQuickUtils &) = delete;

    static void registerDeclarativeType();

    const QLocale &locale() const;

signals:
    void localeChanged();

private:
    StudioQuickUtils();

    QLocale m_locale;
};

} // namespace QmlDesigner
