// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef TRAFFICLIGHT_H
#define TRAFFICLIGHT_H

#include <QWidget>

class GlueInterface;

class TrafficLight final : public QWidget
{
public:
    TrafficLight(GlueInterface *iface);
};

#endif // TRAFFICLIGHT_H
