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

#include <QFrame>
#include <QPropertyAnimation>
#include <QToolButton>

QT_FORWARD_DECLARE_CLASS(QPaintEvent)
QT_FORWARD_DECLARE_CLASS(QStackedWidget)
QT_FORWARD_DECLARE_CLASS(QToolBar)

namespace ScxmlEditor {

namespace OutputPane {

class OutputPane;

class PaneTitleButton : public QToolButton
{
    Q_OBJECT
    Q_PROPERTY(int colorOpacity READ colorOpacity WRITE setColorOpacity)

public:
    PaneTitleButton(OutputPane *pane, QWidget *parent = nullptr);

    int colorOpacity()
    {
        return m_colorOpacity;
    }

    void setColorOpacity(int value);
    void startAlert(const QColor &color = QColor(Qt::red));
    void stopAlert();

protected:
    void paintEvent(QPaintEvent *e) override;

private:
    void fadeIn();
    void fadeOut();

    QPropertyAnimation animator;
    int m_colorOpacity = 255;
    QColor m_color;
    int m_animCounter = 0;
};

class OutputTabWidget : public QFrame
{
    Q_OBJECT

public:
    explicit OutputTabWidget(QWidget *parent = nullptr);
    ~OutputTabWidget();

    int addPane(OutputPane *pane);
    void showPane(OutputPane *pane);
    void showPane(int index);

signals:
    void visibilityChanged(bool visible);

private:
    void createUi();
    void close();
    void buttonClicked(bool para);
    void showAlert();

    QVector<OutputPane*> m_pages;
    QVector<PaneTitleButton*> m_buttons;

    QToolBar *m_toolBar = nullptr;
    QStackedWidget *m_stackedWidget = nullptr;
};

} // namespace OutputPane
} // namespace ScxmlEditor
