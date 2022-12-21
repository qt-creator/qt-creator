// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMenu>
#include <QToolButton>
#include <QWidgetAction>

namespace ScxmlEditor {

namespace Common {

class ColorPickerAction : public QWidgetAction
{
    Q_OBJECT

public:
    ColorPickerAction(const QString &key, QObject *parent);

protected:
    QWidget *createWidget(QWidget *parent) override;

signals:
    void colorSelected(const QString &colorName);
    void lastUsedColor(const QString &colorName);

private:
    QString m_key;
};

class ColorToolButton : public QToolButton
{
    Q_OBJECT

public:
    ColorToolButton(const QString &key, const QString &iconName, const QString &tooltip, QWidget *parent = nullptr);
    ~ColorToolButton() override;

protected:
    void paintEvent(QPaintEvent *e) override;

signals:
    void colorSelected(const QString &colorName);

private:
    void autoColorSelected();
    void showColorDialog();
    void setCurrentColor(const QString &currentColor);

    ColorPickerAction *m_colorPickerAction;
    QString m_color;
    QMenu *m_menu;
};

} // namespace Common
} // namespace ScxmlEditor
