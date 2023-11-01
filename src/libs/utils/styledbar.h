// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QWidget>

namespace Utils {

class QTCREATOR_UTILS_EXPORT StyledBar : public QWidget
{
public:
    StyledBar(QWidget *parent = nullptr);
    void setSingleRow(bool singleRow);
    bool isSingleRow() const;

    void setLightColored(bool lightColored);
    bool isLightColored() const;

protected:
    void paintEvent(QPaintEvent *event) override;
};

class QTCREATOR_UTILS_EXPORT StyledSeparator : public QWidget
{
public:
    StyledSeparator(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
};

} // Utils
