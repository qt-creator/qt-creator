// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QLabel>

namespace Utils {

class  QTCREATOR_UTILS_EXPORT FixedSizeClickLabel : public QLabel
{
    Q_OBJECT
    Q_PROPERTY(QString maxText READ maxText WRITE setMaxText DESIGNABLE true)

public:
    explicit FixedSizeClickLabel(QWidget *parent = nullptr);

    void setText(const QString &text, const QString &maxText);
    using QLabel::setText;
    QSize sizeHint() const override;

    QString maxText() const;
    void setMaxText(const QString &maxText);

protected:
    void mousePressEvent(QMouseEvent *ev) override;
    void mouseReleaseEvent(QMouseEvent *ev) override;

signals:
    void clicked();

private:
    QString m_maxText;
    bool m_pressed = false;
};

} // namespace Utils
