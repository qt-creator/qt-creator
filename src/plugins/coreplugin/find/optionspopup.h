// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../core_global.h"

#include <utils/id.h>

#include <QWidget>

namespace Core {

class CORE_EXPORT OptionsPopup : public QWidget
{
public:
    OptionsPopup(QWidget *parent, const QVector<Utils::Id> &commands);

protected:
    bool event(QEvent *ev) override;
    bool eventFilter(QObject *obj, QEvent *ev) override;
};

} // namespace Core
