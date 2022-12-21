// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QToolButton>

QT_BEGIN_NAMESPACE
class QVBoxLayout;
QT_END_NAMESPACE

namespace Core {
namespace Internal {

class FancyToolButton : public QToolButton
{
    Q_OBJECT

    Q_PROPERTY(qreal fader READ fader WRITE setFader)

public:
    FancyToolButton(QAction *action, QWidget *parent = nullptr);

    void paintEvent(QPaintEvent *event) override;
    bool event(QEvent *e) override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    qreal fader() const { return m_fader; }
    void setFader(qreal value)
    {
        m_fader = value;
        update();
    }

    void setIconsOnly(bool iconsOnly);

    static void hoverOverlay(QPainter *painter, const QRect &spanRect);

private:
    void actionChanged();

    qreal m_fader = 0;
    bool m_iconsOnly = false;
};

class FancyActionBar : public QWidget
{
    Q_OBJECT

public:
    FancyActionBar(QWidget *parent = nullptr);

    void paintEvent(QPaintEvent *event) override;
    void insertAction(int index, QAction *action);
    void addProjectSelector(QAction *action);
    QLayout *actionsLayout() const;
    QSize minimumSizeHint() const override;
    void setIconsOnly(bool iconsOnly);

private:
    QVBoxLayout *m_actionsLayout;
    bool m_iconsOnly = false;
};

} // namespace Internal
} // namespace Core
