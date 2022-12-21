// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    ~OutputTabWidget() override;

    int addPane(OutputPane *pane);
    void showPane(OutputPane *pane);
    void showPane(int index);

signals:
    void visibilityChanged(bool visible);

private:
    void createUi();
    void close();
    void buttonClicked(PaneTitleButton *button, bool checked);
    void showAlert(OutputPane *pane);

    QVector<OutputPane *> m_pages;
    QVector<PaneTitleButton *> m_buttons;

    QToolBar *m_toolBar = nullptr;
    QStackedWidget *m_stackedWidget = nullptr;
};

} // namespace OutputPane
} // namespace ScxmlEditor
