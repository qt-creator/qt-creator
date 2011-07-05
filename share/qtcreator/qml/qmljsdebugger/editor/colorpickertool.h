/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef COLORPICKERTOOL_H
#define COLORPICKERTOOL_H

#include "abstractliveedittool.h"

#include <QtGui/QColor>

QT_FORWARD_DECLARE_CLASS(QPoint)

namespace QmlJSDebugger {

class ColorPickerTool : public AbstractLiveEditTool
{
    Q_OBJECT
public:
    explicit ColorPickerTool(QDeclarativeViewInspector *view);

    virtual ~ColorPickerTool();

    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);

    void hoverMoveEvent(QMouseEvent *event);

    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *keyEvent);

    void wheelEvent(QWheelEvent *event);

    void itemsAboutToRemoved(const QList<QGraphicsItem*> &itemList);

    void clear();

signals:
    void selectedColorChanged(const QColor &color);

protected:

    void selectedItemsChanged(const QList<QGraphicsItem*> &itemList);

private:
    void pickColor(const QPoint &pos);

private:
    QColor m_selectedColor;
};

} // namespace QmlJSDebugger

#endif // COLORPICKERTOOL_H
