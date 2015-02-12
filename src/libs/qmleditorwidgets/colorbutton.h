/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef COLORBUTTON_H
#define COLORBUTTON_H

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
    ColorButton(QWidget *parent = 0) :
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
    void paintEvent(QPaintEvent *event);
private:
    QString m_colorString;
    bool m_noColor;
    bool m_showArrow;
};

} //QmlEditorWidgets

#endif //COLORBUTTON_H
