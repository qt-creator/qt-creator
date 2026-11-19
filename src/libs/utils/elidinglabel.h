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

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    bool event(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    bool focusNextPrevChild(bool next) override;
    void updateToolTip(const QString& elidedText);

private:
    void followLinkHandler();
    bool handlesLinksItself() const;
    bool handlesLinkFocusItself() const;
    void activateLink(const QString &link);

    Qt::TextElideMode m_elideMode;
    QString m_additionalToolTip;
    QString m_hoveredLink;
    QString m_pressedLink;
    int m_focusedAnchor = -1;
    bool m_paintsRichTextItself = false;
    QString m_additionalToolTipSeparator{"\n\n"};
};

} // namespace Utils
