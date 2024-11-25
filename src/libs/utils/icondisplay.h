// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <memory>

#include <QWidget>

QT_BEGIN_NAMESPACE
class QPaintEvent;
QT_END_NAMESPACE

namespace Utils {

class Icon;
class IconDisplayPrivate;

class QTCREATOR_UTILS_EXPORT IconDisplay : public QWidget
{
    Q_OBJECT
public:
    IconDisplay(QWidget *parent = nullptr);
    ~IconDisplay() override;

    void setIcon(const Utils::Icon &);

    void paintEvent(QPaintEvent *event) override;
    QSize sizeHint() const override;

private:
    friend class IconDisplayPrivates;

    std::unique_ptr<IconDisplayPrivate> d;
};

} // namespace Utils
