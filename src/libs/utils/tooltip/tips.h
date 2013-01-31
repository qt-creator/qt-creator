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

#ifndef TIPS_H
#define TIPS_H

#include <QSharedPointer>
#include <QLabel>
#include <QPixmap>

QT_FORWARD_DECLARE_CLASS(QVBoxLayout)

namespace Utils {
class TipContent;
}

#ifndef Q_MOC_RUN
namespace Utils {
namespace Internal {
#endif

// Please do not change the name of this class. Detailed comments in tooltip.h.
class QTipLabel : public QLabel
{
    Q_OBJECT
protected:
    QTipLabel(QWidget *parent);

public:
    virtual ~QTipLabel();

    void setContent(const Utils::TipContent &content);
    const Utils::TipContent &content() const;

    virtual void configure(const QPoint &pos, QWidget *w) = 0;
    virtual bool canHandleContentReplacement(const Utils::TipContent &content) const = 0;

    bool isInteractive() const;

private:
    Utils::TipContent *m_tipContent;
};

class ColorTip : public QTipLabel
{
    Q_OBJECT
public:
    ColorTip(QWidget *parent);
    virtual ~ColorTip();

    virtual void configure(const QPoint &pos, QWidget *w);
    virtual bool canHandleContentReplacement(const TipContent &content) const;

private:
    virtual void paintEvent(QPaintEvent *event);

    QPixmap m_tilePixMap;
};

class TextTip : public QTipLabel
{
    Q_OBJECT
public:
    TextTip(QWidget *parent);
    virtual ~TextTip();

    virtual void configure(const QPoint &pos, QWidget *w);
    virtual bool canHandleContentReplacement(const TipContent &content) const;

private:
    virtual void paintEvent(QPaintEvent *event);
    virtual void resizeEvent(QResizeEvent *event);
};

class WidgetTip : public QTipLabel
{
    Q_OBJECT
public:
    explicit WidgetTip(QWidget *parent = 0);

    virtual void configure(const QPoint &pos, QWidget *w);
    virtual bool canHandleContentReplacement(const TipContent &content) const;

public slots:
    void pinToolTipWidget();

private:
    QWidget *takeWidget(Qt::WindowFlags wf = 0);

    QVBoxLayout *m_layout;
};

#ifndef Q_MOC_RUN
} // namespace Internal
} // namespace Utils
#endif

#endif // TIPS_H
