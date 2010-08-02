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

#ifndef TOOLTIP_H
#define TOOLTIP_H

#include "texteditor/texteditor_global.h"

#include <QtCore/QSharedPointer>
#include <QtCore/QObject>
#include <QtCore/QTimer>

QT_BEGIN_NAMESPACE
class QPoint;
class QString;
class QColor;
class QWidget;
QT_END_NAMESPACE

namespace TextEditor {

namespace Internal {
class Tip;
class TipContent;
}

/*
 * This class contains some code duplicated from QTooltip. It would be good to make that reusable.
 */

class TEXTEDITOR_EXPORT ToolTip : public QObject
{
    Q_OBJECT
private:
    ToolTip();

public:
    virtual ~ToolTip();

    static ToolTip *instance();

    void showText(const QPoint &pos, const QString &text, QWidget *w = 0);
    void showColor(const QPoint &pos, const QColor &color, QWidget *w = 0);
    void hide();
    bool isVisible() const;

    virtual bool eventFilter(QObject *o, QEvent *event);

private:
    bool acceptShow(const QSharedPointer<Internal::TipContent> &content,
                    const QPoint &pos,
                    QWidget *w);
    void setUp(const QSharedPointer<Internal::TipContent> &content,
               const QPoint &pos,
               QWidget *w);
    bool requiresSetUp(const QSharedPointer<Internal::TipContent> &content, QWidget *w) const;
    void placeTip(const QPoint &pos, QWidget *w);
    int tipScreen(const QPoint &pos, QWidget *w) const;
    void showTip();
    void hideTipWithDelay();
    void hideQtTooltip();

private slots:
    void hideTipImmediately();

private:
    Internal::Tip *m_tip;
    QWidget *m_widget;
    QTimer m_showTimer;
    QTimer m_hideDelayTimer;
};

} // namespace TextEditor

#endif // TOOLTIP_H
