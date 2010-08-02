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

#ifndef TIPS_H
#define TIPS_H

#include <QtCore/QSharedPointer>
#include <QtGui/QFrame>
#include <QtGui/QPixmap>

QT_BEGIN_NAMESPACE
class QColor;
QT_END_NAMESPACE

namespace TextEditor {
namespace Internal {

class TipContent;

class Tip : public QFrame
{
    Q_OBJECT
protected:
    Tip(QWidget *parent);

public:
    virtual ~Tip();

    void setContent(const QSharedPointer<TipContent> &content);
    const QSharedPointer<TipContent> &content() const;

private:
    virtual void configure() = 0;

    QSharedPointer<TipContent> m_content;
};

class ColorTip : public Tip
{
    Q_OBJECT
public:
    ColorTip(QWidget *parent);
    virtual ~ColorTip();

private:
    virtual void configure();
    virtual void paintEvent(QPaintEvent *event);

    QPixmap m_tilePixMap;
};

} // namespace Internal
} // namespace TextEditor

#endif // TIPS_H
