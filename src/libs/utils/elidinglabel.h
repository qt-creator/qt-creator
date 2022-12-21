// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QLabel>

namespace Utils {

class  QTCREATOR_UTILS_EXPORT ElidingLabel : public QLabel
{
    Q_OBJECT
    Q_PROPERTY(Qt::TextElideMode elideMode READ elideMode WRITE setElideMode DESIGNABLE true)
    Q_PROPERTY(QString additionalToolTip READ additionalToolTip WRITE setAdditionalToolTip DESIGNABLE true)
    Q_PROPERTY(QString additionalToolTipSeparator READ additionalToolTipSeparator WRITE setAdditionalToolTipSeparator DESIGNABLE true)

public:
    explicit ElidingLabel(QWidget *parent = nullptr);
    explicit ElidingLabel(const QString &text, QWidget *parent = nullptr);

    Qt::TextElideMode elideMode() const;
    void setElideMode(const Qt::TextElideMode &elideMode);

    QString additionalToolTip();
    void setAdditionalToolTip(const QString& additionalToolTip);

    QString additionalToolTipSeparator() const;
    void setAdditionalToolTipSeparator(const QString &newAdditionalToolTipSeparator);

protected:
    void paintEvent(QPaintEvent *event) override;
    void updateToolTip(const QString& elidedText);

private:
    Qt::TextElideMode m_elideMode;
    QString m_additionalToolTip;
    QString m_additionalToolTipSeparator{"\n\n"};
};

} // namespace Utils
