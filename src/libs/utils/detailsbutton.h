// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QCoreApplication>
#include <QToolButton>

QT_BEGIN_NAMESPACE
class QGraphicsOpacityEffect;
QT_END_NAMESPACE

namespace Utils {
class QTCREATOR_UTILS_EXPORT FadingPanel : public QWidget
{
public:
    FadingPanel(QWidget *parent = nullptr)
        : QWidget(parent)
    {}
    virtual void fadeTo(qreal value) = 0;
    virtual void setOpacity(qreal value) = 0;
};

class QTCREATOR_UTILS_EXPORT FadingWidget : public FadingPanel
{
public:
    FadingWidget(QWidget *parent = nullptr);
    void fadeTo(qreal value) override;
    qreal opacity();
    void setOpacity(qreal value) override;
protected:
    QGraphicsOpacityEffect *m_opacityEffect;
};

class QTCREATOR_UTILS_EXPORT ExpandButton : public QToolButton
{
public:
    ExpandButton(QWidget *parent = nullptr);
};

class QTCREATOR_UTILS_EXPORT DetailsButton : public ExpandButton
{
public:
    DetailsButton(QWidget *parent = nullptr);
    QSize sizeHint() const override;
    static QColor outlineColor();

private:
    void paintEvent(QPaintEvent *e) override;
    const int spacing = 6;
};

} // namespace Utils
