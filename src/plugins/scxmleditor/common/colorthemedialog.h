// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>

namespace ScxmlEditor {

namespace Common {

class ColorSettings;

class ColorThemeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ColorThemeDialog(QWidget *parent = nullptr);

    void save();

private:
    ColorSettings *m_colorSettings;
};

} // namespace Common
} // namespace ScxmlEditor
