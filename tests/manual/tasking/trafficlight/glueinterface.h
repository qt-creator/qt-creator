// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef GLUEINTERFACE_H
#define GLUEINTERFACE_H

#include <QObject>

enum class Light
{
    Off    = 0,
    Red    = 1 << 0,
    Yellow = 1 << 1,
    Green  = 1 << 2,
};

Q_DECLARE_FLAGS(Lights, Light)
Q_DECLARE_OPERATORS_FOR_FLAGS(Lights)

class GlueInterface final : public QObject
{
    Q_OBJECT

public:
    // business logic -> GUI
    void setLights(Lights lights) { emit lightsChanged(lights); }

    // GUI -> business logic
    void smash() { emit smashed(); }
    void repair() { emit repaired(); }

signals:
    void lightsChanged(Lights lights);

    void smashed();
    void repaired();
};

#endif // GLUEINTERFACE_H
