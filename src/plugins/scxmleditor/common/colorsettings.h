// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "ui_colorsettings.h"
#include <QFrame>

namespace ScxmlEditor {
namespace Common {

class ColorSettings : public QFrame
{
    Q_OBJECT

public:
    explicit ColorSettings(QWidget *parent = nullptr);

    void save();
    void updateCurrentColors();
    void createTheme();
    void removeTheme();

private:
    void selectTheme(int);

    QVariantMap m_colorThemes;
    Ui::ColorSettings m_ui;
};

} // namespace Common
} // namespace ScxmlEditor
