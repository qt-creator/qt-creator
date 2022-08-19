// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QAbstractButton>

QT_BEGIN_NAMESPACE
class QGraphicsOpacityEffect;
QT_END_NAMESPACE

namespace Utils {
class QTCREATOR_UTILS_EXPORT FadingPanel : public QWidget
{
    Q_OBJECT

public:
    FadingPanel(QWidget *parent = nullptr)
        : QWidget(parent)
    {}
    virtual void fadeTo(qreal value) = 0;
    virtual void setOpacity(qreal value) = 0;
};

class QTCREATOR_UTILS_EXPORT FadingWidget : public FadingPanel
{
    Q_OBJECT
public:
    FadingWidget(QWidget *parent = nullptr);
    void fadeTo(qreal value) override;
    qreal opacity();
    void setOpacity(qreal value) override;
protected:
    QGraphicsOpacityEffect *m_opacityEffect;
};

class QTCREATOR_UTILS_EXPORT DetailsButton : public QAbstractButton
{
    Q_OBJECT
    Q_PROPERTY(float fader READ fader WRITE setFader)

public:
    DetailsButton(QWidget *parent = nullptr);

    QSize sizeHint() const override;
    float fader() { return m_fader; }
    void setFader(float value) { m_fader = value; update(); }

protected:
    void paintEvent(QPaintEvent *e) override;
    bool event(QEvent *e) override;
    void changeEvent(QEvent *e) override;

private:
    QPixmap cacheRendering(const QSize &size, bool checked);
    QPixmap m_checkedPixmap;
    QPixmap m_uncheckedPixmap;
    float m_fader;
};

class QTCREATOR_UTILS_EXPORT ExpandButton : public QAbstractButton
{
    Q_OBJECT

public:
    ExpandButton(QWidget *parent = nullptr);

private:
    void paintEvent(QPaintEvent *e) override;
    QSize sizeHint() const override;

    QPixmap cacheRendering();

    QPixmap m_checkedPixmap;
    QPixmap m_uncheckedPixmap;
};

} // namespace Utils
