// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QAbstractButton>

QT_BEGIN_NAMESPACE
class QKeyEvent;
class QPaintEvent;
QT_END_NAMESPACE

namespace Utils {

class QTCREATOR_UTILS_EXPORT FancyIconButton : public QAbstractButton
{
    Q_OBJECT
    Q_PROPERTY(float iconOpacity READ iconOpacity WRITE setIconOpacity)
    Q_PROPERTY(bool autoHide READ hasAutoHide WRITE setAutoHide)
public:
    explicit FancyIconButton(QWidget *parent = nullptr);
    void paintEvent(QPaintEvent *event) override;
    float iconOpacity() { return m_iconOpacity; }
    void setIconOpacity(float value) { m_iconOpacity = value; update(); }
    void animateShow(bool visible);

    void setAutoHide(bool hide) { m_autoHide = hide; }
    bool hasAutoHide() const { return m_autoHide; }

    QSize sizeHint() const override;

protected:
    void keyPressEvent(QKeyEvent *ke) override;
    void keyReleaseEvent(QKeyEvent *ke) override;

private:
    float m_iconOpacity = 1.0f;
    bool m_autoHide = false;
};

} // namespace Utils
