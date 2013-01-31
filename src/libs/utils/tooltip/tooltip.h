/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef TOOLTIP_H
#define TOOLTIP_H

#include "../utils_global.h"

#include <QSharedPointer>
#include <QObject>
#include <QTimer>
#include <QRect>
#include <QFont>

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

namespace Utils {
namespace Internal { class QTipLabel; }

class TipContent;

class QTCREATOR_UTILS_EXPORT ToolTip : public QObject
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

    Internal::QTipLabel *m_tip;
    QWidget *m_widget;
    QRect m_rect;
    QTimer m_showTimer;
    QTimer m_hideDelayTimer;
};

} // namespace Utils

#endif // TOOLTIP_H
