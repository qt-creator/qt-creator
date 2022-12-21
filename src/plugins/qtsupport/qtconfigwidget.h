// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qtsupport_global.h"

#include <QWidget>

namespace QtSupport {

class QTSUPPORT_EXPORT QtConfigWidget : public QWidget
{
    Q_OBJECT
public:
    QtConfigWidget();
signals:
    void changed();
};
} // namespace QtSupport
