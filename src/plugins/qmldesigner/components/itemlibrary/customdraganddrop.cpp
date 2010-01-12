/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "customdraganddrop.h"

#include <QtCore/QMimeData>
#include <QtCore/QPoint>
#include <QLabel>

#include <QMouseEvent>
#include <QApplication>
#include <QDebug>
#include <QPainter>

namespace QmlDesigner {

namespace QmlDesignerItemLibraryDragAndDrop {

void CustomDragAndDropIcon::startDrag()
{
    m_size = m_icon.size();
    setPixmap(m_icon);
    m_iconAlpha = 1;
    m_previewAlpha = 0;

    QPoint pos = QCursor::pos();
    QWidget *widget = QApplication::topLevelAt(pos);
    setParent(widget);
    raise();
    show();

    grabMouse();
}

void CustomDragAndDropIcon::mouseReleaseEvent(QMouseEvent *event)
{
     QPoint globalPos = event->globalPos();
     releaseMouse();
     move(-1000, -1000); //-1000, -1000 is used to hide because hiding causes propblems with the mouse grabber
     QWidget* target = QApplication::widgetAt(globalPos - QPoint(2,2)); //-(2, 2) because:
                                                                        // otherwise we just get this widget

     if (CustomDragAndDrop::isAccepted()) // if the target widget accepted the enter event,
                                          // then create a drop event
         CustomDragAndDrop::drop(target, globalPos);
     CustomDragAndDrop::endCustomDrag();
}

void CustomDragAndDropIcon::mouseMoveEvent(QMouseEvent *event)
{
    QPoint globalPos = event->globalPos();
    QWidget * widget = QApplication::topLevelAt(globalPos); //grap the "current" top level widget
    if (widget) {
        setParent(widget); //set the current top level widget as parent
        QPoint pos = parentWidget()->mapFromGlobal(globalPos);
        if ((pos.y() > 30 && CustomDragAndDrop::isVisible())) //do not allow dragging over the menubar
            move(pos);
        else
            move(-1000, -1000); //no hiding because of mouse grabbing
        setPixmap(currentImage());
        resize(m_size);
        show();
        update();
    }
    else {
       move(-1000, -1000); //if no top level widget is found we are out of the main window
    }
    QWidget* target = QApplication::widgetAt(globalPos - QPoint(2,2)); //-(2, 2) because:
                                                                       // otherwise we just get this widget
    if (target != m_oldTarget) {
      if (CustomDragAndDrop::isAccepted())
        CustomDragAndDrop::leave(m_oldTarget, globalPos);  // create DragLeave event if drag enter
                                                           // event was accepted
      bool wasAccepted = CustomDragAndDrop::isAccepted();
      CustomDragAndDrop::enter(target, globalPos);         // create and handle the create enter event
      releaseMouse(); //to set the cursor we have to disable the mouse grabber
      if (CustomDragAndDrop::isAccepted()) { //setting the right cursor and trigger animation
          setCursor(Qt::CrossCursor);
          if (!wasAccepted)
              enter();                      //trigger animation if enter event was accepted
      } else {
          setCursor(Qt::ForbiddenCursor);
          if (wasAccepted)
              leave();                     // trigger animation if we leave a widget that accepted
                                           // the drag enter event
      }
      grabMouse(); //enable the mouse grabber again - after the curser is set
    } else {
        if (CustomDragAndDrop::isAccepted()) // create DragMoveEvents if the current widget
                                             // accepted the DragEnter event
            CustomDragAndDrop::move(target, globalPos);
    }
    m_oldTarget = target;
}


QPixmap CustomDragAndDropIcon::currentImage()
{
    //blend the two images (icon and preview) according to alpha values
    QPixmap pixmap(m_size);
    pixmap.fill(Qt::white);
    QPainter p(&pixmap);
    if  (CustomDragAndDrop::isAccepted()) {
        p.setOpacity(m_previewAlpha);
        p.drawPixmap(0 ,0 , m_size.width(), m_size.height(), m_preview);
        p.setOpacity(m_iconAlpha);
        p.drawPixmap(0, 0, m_size.width(), m_size.height(), m_icon);
    } else {
        p.setOpacity(m_iconAlpha);
        p.drawPixmap(0, 0, m_size.width(), m_size.height(), m_icon);
        p.setOpacity(m_previewAlpha);
        p.drawPixmap(0 ,0 , m_size.width(), m_size.height(), m_preview);
    }
    return pixmap;
}

void CustomDragAndDropIcon::enter()
{
    connect(&m_timeLine, SIGNAL( frameChanged (int)), this, SLOT(animateDrag(int)));
    m_timeLine.setFrameRange(0, 10);
    m_timeLine.setDuration(250);
    m_timeLine.setLoopCount(1);
    m_timeLine.setCurveShape(QTimeLine::EaseInCurve);
    m_timeLine.start();
    m_size  = m_icon.size();
    m_iconAlpha = 1;
    m_previewAlpha = 0;
}

void CustomDragAndDropIcon::leave()
{
    connect(&m_timeLine, SIGNAL( frameChanged (int)), this, SLOT(animateDrag(int)));
    m_timeLine.setFrameRange(0, 10);
    m_timeLine.setDuration(250);
    m_timeLine.setLoopCount(1);
    m_timeLine.setCurveShape(QTimeLine::EaseInCurve);
    m_timeLine.start();
    m_size  = m_preview.size();
    m_iconAlpha = 0;
    m_previewAlpha = 1;
}

void CustomDragAndDropIcon::animateDrag(int frame)
{
    //interpolation of m_size and alpha values
    if (CustomDragAndDrop::isAccepted()) {
        //small -> big
         m_iconAlpha = 1.0 - qreal(frame) / 10.0;
         m_previewAlpha = qreal(frame) / 10.0;
         int width = qreal(m_preview.width()) * (qreal(frame) / 10.0) + qreal(m_icon.width()) * (1.0 - qreal(frame) / 10.0);
         int height = qreal(m_preview.height()) * (qreal(frame) / 10.0) + qreal(m_icon.height()) * (1 - qreal(frame) / 10.0);
         m_size = QSize(width, height);
    } else {
        //big -> small
         m_previewAlpha = 1.0 - qreal(frame) / 10.0;
         m_iconAlpha = qreal(frame) / 10.0;
         int width = qreal(m_icon.width()) * (qreal(frame) / 10.0) + qreal(m_preview.width()) * (1.0 - qreal(frame) / 10.0);
         int height = qreal(m_icon.height()) * (qreal(frame) / 10.0) + qreal(m_preview.height()) * (1 - qreal(frame) / 10.0);
         m_size = QSize(width, height);
    }
    QPoint p = pos();
    setPixmap(currentImage());
    resize(m_size);
    move(p);
    update(); //redrawing needed
}


class CustomDragAndDropGuard { //This guard destroys the singleton in its destructor
public:                   //This should avoid that a memory leak is reported
   ~CustomDragAndDropGuard() {
   if (CustomDragAndDrop::m_instance != 0)
     delete CustomDragAndDrop::m_instance;
   }
};


CustomDragAndDrop* CustomDragAndDrop::m_instance = 0;

CustomDragAndDrop::CustomDragAndDrop() : m_customDragActive(0), m_mimeData(0), m_accepted(false)
{
    m_widget = new CustomDragAndDropIcon(QApplication::topLevelWidgets().first());
    m_widget->move(-1000, 1000);
    m_widget->resize(32, 32);
    m_widget->show();
}


CustomDragAndDrop* CustomDragAndDrop::instance()
{
    static CustomDragAndDropGuard guard; //The destructor destroys the singleton. See above
    if (m_instance == 0)
        m_instance = new CustomDragAndDrop();
    return m_instance;
}

void CustomDragAndDrop::endCustomDrag()
{
    instance()->m_customDragActive = false;
}

void CustomDragAndDrop::startCustomDrag(const QPixmap icon, const QPixmap preview, QMimeData * mimeData)
{
    if (instance()->m_customDragActive) {
        qWarning("CustomDragAndDrop::startCustomDrag drag is active");
        return ;
    }
    instance()->m_customDragActive = true;
    instance()-> m_accepted = false;
    show();
    instance()->m_mimeData = mimeData;
    instance()->m_widget->setIcon(icon);
    instance()->m_widget->setPreview(preview);
    instance()->m_widget->startDrag();

}

bool CustomDragAndDrop::customDragActive()
{
    return instance()->m_customDragActive;
}

bool CustomDragAndDrop::isAccepted()
{
    return instance()->m_accepted;
}

void CustomDragAndDrop::enter(QWidget *target, QPoint globalPos)
{
    if (target) {
        QPoint pos = target->mapFromGlobal(globalPos);
       QDragEnterEvent event(pos, Qt::MoveAction, instance()->m_mimeData ,Qt::RightButton, Qt::NoModifier);
       QApplication::sendEvent(target, &event);
       instance()-> m_accepted = event.isAccepted(); //if the event is accepted we enter the "accepted" state
    } else {
        instance()-> m_accepted = false;
    }
}

void CustomDragAndDrop::leave(QWidget *target, QPoint globalPos)
{
    if (target) {
        QPoint pos = target->mapFromGlobal(globalPos);
        QDragLeaveEvent event;
        QApplication::sendEvent(target, &event);
    } else {
       qWarning() <<  "CustomDragAndDrop::leave leaving invalid target";
    }
}

void CustomDragAndDrop::move(QWidget *target, QPoint globalPos)
{
    if (target) {
        QPoint pos = target->mapFromGlobal(globalPos);
        QDragMoveEvent event(pos, Qt::MoveAction, instance()->m_mimeData ,Qt::RightButton, Qt::NoModifier);
        QApplication::sendEvent(target, &event);
    } else {
        qWarning() <<  "CustomDragAndDrop::move move in invalid target";
    }
}

void CustomDragAndDrop::drop(QWidget *target, QPoint globalPos)
{
    if (target) {
        QPoint pos = target->mapFromGlobal(globalPos);
        QDropEvent event(pos, Qt::MoveAction, instance()->m_mimeData ,Qt::RightButton, Qt::NoModifier);
        QApplication::sendEvent(target, &event);
    } else {
        qWarning() <<  "CustomDragAndDrop::drop dropping in invalid target";
    }
}

} //namespace QmlDesignerItemLibraryDragAndDrop
} //namespave QmlDesigner

