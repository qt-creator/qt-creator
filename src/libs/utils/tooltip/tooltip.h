/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "../utils_global.h"

#include <QSharedPointer>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QRect>

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
class QVariant;
class QLayout;
class QWidget;
QT_END_NAMESPACE

namespace Utils {
namespace Internal { class QTipLabel; }

class QTCREATOR_UTILS_EXPORT ToolTip : public QObject
{
    Q_OBJECT
protected:
    ToolTip();

public:
    ~ToolTip();

    enum {
        ColorContent = 0,
        TextContent = 1,
        WidgetContent = 42
    };

    bool eventFilter(QObject *o, QEvent *event);

    static ToolTip *instance();

    static void show(const QPoint &pos, const QString &content, QWidget *w = 0,
                     const QString &helpId = QString(), const QRect &rect = QRect());
    static void show(const QPoint &pos, const QColor &color, QWidget *w = 0,
                     const QString &helpId = QString(), const QRect &rect = QRect());
    static void show(const QPoint &pos, QWidget *content, QWidget *w = 0,
                     const QString &helpId = QString(), const QRect &rect = QRect());
    static void show(const QPoint &pos, QLayout *content, QWidget *w = 0,
                     const QString &helpId = QString(), const QRect &rect = QRect());
    static void move(const QPoint &pos, QWidget *w);
    static void hide();
    static void hideImmediately();
    static bool isVisible();

    static QPoint offsetFromPosition();

    // Helper to 'pin' (show as real window) a tooltip shown
    // using WidgetContent
    static bool pinToolTip(QWidget *w, QWidget *parent);

    static QString contextHelpId();

signals:
    void shown();
    void hidden();

private:
    void showInternal(const QPoint &pos, const QVariant &content, int typeId, QWidget *w,
                      const QString &helpId, const QRect &rect);
    void hideTipImmediately();
    bool acceptShow(const QVariant &content, int typeId, const QPoint &pos, QWidget *w,
                    const QString &helpId, const QRect &rect);
    void setUp(const QPoint &pos, QWidget *w, const QRect &rect);
    bool tipChanged(const QPoint &pos, const QVariant &content, int typeId, QWidget *w,
                    const QString &helpId) const;
    void setTipRect(QWidget *w, const QRect &rect);
    void placeTip(const QPoint &pos, QWidget *w);
    void showTip();
    void hideTipWithDelay();

    QPointer<Internal::QTipLabel> m_tip;
    QWidget *m_widget;
    QRect m_rect;
    QTimer m_showTimer;
    QTimer m_hideDelayTimer;
    QString m_helpId;
};

} // namespace Utils
