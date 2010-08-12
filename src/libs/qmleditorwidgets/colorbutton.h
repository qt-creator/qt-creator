/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef COLORBUTTON_H
#define COLORBUTTON_H

#include <qmleditorwidgets_global.h>
#include <QtGui/QWidget>
#include <QToolButton>
#include <qdeclarative.h>

namespace QmlEditorWidgets {

class QMLEDITORWIDGETS_EXPORT ColorButton : public QToolButton {

Q_OBJECT

Q_PROPERTY(QString color READ color WRITE setColor NOTIFY colorChanged)
Q_PROPERTY(bool noColor READ noColor WRITE setNoColor)
Q_PROPERTY(bool showArrow READ showArrow WRITE setShowArrow)

public:
    ColorButton(QWidget *parent = 0) : QToolButton (parent), m_colorString("#ffffff"), m_noColor(false), m_showArrow(true) {}

    void setColor(const QString &colorStr);
    QString color() const { return m_colorString; }
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

QML_DECLARE_TYPE(QmlEditorWidgets::ColorButton);

#endif //COLORBUTTON_H
