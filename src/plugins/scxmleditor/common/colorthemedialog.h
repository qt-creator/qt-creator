// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "ui_colorthemedialog.h"
#include <QDialog>

namespace ScxmlEditor {

namespace Common {

class ColorThemeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ColorThemeDialog(QWidget *parent = nullptr);

    void save();

private:
    Ui::ColorThemeDialog m_ui;
};

} // namespace Common
} // namespace ScxmlEditor
