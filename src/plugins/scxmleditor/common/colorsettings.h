// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QFrame>
#include <QVariantMap>

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

namespace ScxmlEditor {
namespace Common {

class ColorThemeView;

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
    ColorThemeView *m_colorThemeView;
    QComboBox *m_comboColorThemes;
};

} // namespace Common
} // namespace ScxmlEditor
