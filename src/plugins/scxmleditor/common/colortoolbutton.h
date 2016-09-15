/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
