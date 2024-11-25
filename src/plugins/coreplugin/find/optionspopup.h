// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../core_global.h"

#include <utils/id.h>

#include <QWidget>

#include <optional>

namespace Core {

class CORE_EXPORT NumericOption
{
public:
    NumericOption() noexcept = default;
    NumericOption(int current, int minimum, int maximum) noexcept
        : currentValue{current}
        , minimumValue{minimum}
        , maximumValue{maximum}
    {}

    static std::optional<NumericOption> get(QObject *o);
    static void set(QObject *o, const NumericOption &opt);

    int currentValue;
    int minimumValue;
    int maximumValue;
};

class CORE_EXPORT OptionsPopup : public QWidget
{
public:
    OptionsPopup(QWidget *parent, const QVector<Utils::Id> &commands);

protected:
    bool event(QEvent *ev) override;
    bool eventFilter(QObject *obj, QEvent *ev) override;
};

} // namespace Core
