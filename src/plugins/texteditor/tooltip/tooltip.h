/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef TOOLTIP_H
#define TOOLTIP_H

#include "texteditor/texteditor_global.h"

#include <QtCore/QSharedPointer>
#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QRect>
#include <QtGui/QFont>

/*
 * In its current form QToolTip is not extensible. So this is an attempt to provide a more
 * flexible and customizable tooltip mechanism for Creator. Part of the code here is duplicated
 * from QToolTip. This includes a private Qt header and the non-exported class QTipLabel, which
 * here serves as a base tip class. Please notice that Qt relies on this particular class name in
 * order to correctly apply the native styles for tooltips. Therefore the QTipLabel name should
 * not be changed.
 */

QT_BEGIN_NAMESPACE
class QPoint;
class QWidget;
QT_END_NAMESPACE

namespace TextEditor {

namespace Internal {
class TipFactory;
class QTipLabel;
}

class TipContent;

class TEXTEDITOR_EXPORT ToolTip : public QObject
{
    Q_OBJECT
private:
    ToolTip();

public:
    virtual ~ToolTip();

    static ToolTip *instance();

    void show(const QPoint &pos, const TipContent &content, QWidget *w = 0);
    void show(const QPoint &pos, const TipContent &content, QWidget *w, const QRect &rect);
    void hide();
    bool isVisible() const;

    QFont font() const;
    void setFont(const QFont &font);

    virtual bool eventFilter(QObject *o, QEvent *event);

private slots:
    void hideTipImmediately();

private:
    bool acceptShow(const TipContent &content, const QPoint &pos, QWidget *w, const QRect &rect);
    bool validateContent(const TipContent &content);
    void setUp(const QPoint &pos, const TipContent &content, QWidget *w, const QRect &rect);
    bool tipChanged(const QPoint &pos, const TipContent &content, QWidget *w) const;
    void setTipRect(QWidget *w, const QRect &rect);
    void placeTip(const QPoint &pos, QWidget *w);
    void showTip();
    void hideTipWithDelay();

    Internal::TipFactory *m_tipFactory;
    Internal::QTipLabel *m_tip;
    QWidget *m_widget;
    QRect m_rect;
    QTimer m_showTimer;
    QTimer m_hideDelayTimer;
};

} // namespace TextEditor

#endif // TOOLTIP_H
