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
#ifndef TOOLBARCOLORBOX_H
#define TOOLBARCOLORBOX_H

#include <QLabel>
#include <QColor>
#include <QPoint>

QT_FORWARD_DECLARE_CLASS(QContextMenuEvent);
QT_FORWARD_DECLARE_CLASS(QAction);

namespace QmlJSInspector {

class ToolBarColorBox : public QLabel
{
    Q_OBJECT
public:
    explicit ToolBarColorBox(QWidget *parent = 0);
    void setColor(const QColor &color);
    void setInnerBorderColor(const QColor &color);
    void setOuterBorderColor(const QColor &color);

protected:
    void contextMenuEvent(QContextMenuEvent *ev);
    void mouseDoubleClickEvent(QMouseEvent *);
    void mousePressEvent(QMouseEvent *ev);
    void mouseMoveEvent(QMouseEvent *ev);
private slots:
    void copyColorToClipboard();

private:
    QPixmap createDragPixmap(int size = 24) const;

private:
    bool m_dragStarted;
    QPoint m_dragBeginPoint;
    QAction *m_copyHexColorAction;
    QColor m_color;

    QColor m_borderColorOuter;
    QColor m_borderColorInner;

};

} // namespace QmlJSInspector

#endif // TOOLBARCOLORBOX_H
