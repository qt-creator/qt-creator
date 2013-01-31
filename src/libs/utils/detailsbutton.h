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

#ifndef DETAILSBUTTON_H
#define DETAILSBUTTON_H

#include "utils_global.h"

#include <QAbstractButton>
#include <QPixmap>

#include <QGraphicsOpacityEffect>

QT_FORWARD_DECLARE_CLASS(QGraphicsOpacityEffect)

namespace Utils {
class QTCREATOR_UTILS_EXPORT FadingPanel : public QWidget
{
    Q_OBJECT

public:
    FadingPanel(QWidget *parent = 0)
        : QWidget(parent)
    {}
    virtual void fadeTo(qreal value) = 0;
    virtual void setOpacity(qreal value) = 0;
};

class QTCREATOR_UTILS_EXPORT FadingWidget : public FadingPanel
{
    Q_OBJECT
public:
    FadingWidget(QWidget *parent = 0);
    void fadeTo(qreal value);
    qreal opacity();
    void setOpacity(qreal value);
protected:
    QGraphicsOpacityEffect *m_opacityEffect;
};

class QTCREATOR_UTILS_EXPORT DetailsButton : public QAbstractButton
{
    Q_OBJECT
    Q_PROPERTY(float fader READ fader WRITE setFader)

public:
    DetailsButton(QWidget *parent = 0);

    QSize sizeHint() const;
    float fader() { return m_fader; }
    void setFader(float value) { m_fader = value; update(); }

protected:
    void paintEvent(QPaintEvent *e);
    bool event(QEvent *e);

private:
    QPixmap cacheRendering(const QSize &size, bool checked);
    QPixmap m_checkedPixmap;
    QPixmap m_uncheckedPixmap;
    float m_fader;
};
} // namespace Utils
#endif // DETAILSBUTTON_H
