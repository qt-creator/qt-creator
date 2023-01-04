// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmleditorwidgets_global.h"
#include <QToolButton>
#include <QVariant>

namespace QmlEditorWidgets {

class QMLEDITORWIDGETS_EXPORT ColorButton : public QToolButton {

Q_OBJECT

Q_PROPERTY(QVariant color READ color WRITE setColor NOTIFY colorChanged)
Q_PROPERTY(bool noColor READ noColor WRITE setNoColor)
Q_PROPERTY(bool showArrow READ showArrow WRITE setShowArrow)

public:
    ColorButton(QWidget *parent = nullptr) :
        QToolButton (parent),
        m_colorString(QLatin1String("#ffffff")),
        m_noColor(false),
        m_showArrow(true)
    {}

    void setColor(const QVariant &colorStr);
    QVariant color() const { return m_colorString; }
    QColor convertedColor() const;
    bool noColor() const { return m_noColor; }
    void setNoColor(bool f) { m_noColor = f; update(); }
    bool showArrow() const { return m_showArrow; }
    void setShowArrow(bool b) { m_showArrow = b; }

signals:
    void colorChanged();

protected:
    void paintEvent(QPaintEvent *event) override;
private:
    QString m_colorString;
    bool m_noColor;
    bool m_showArrow;
};

} //QmlEditorWidgets
