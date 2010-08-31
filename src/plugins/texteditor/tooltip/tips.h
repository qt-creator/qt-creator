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
#include <QtGui/QLabel>
#include <QtGui/QPixmap>

namespace TextEditor {
class TipContent;
}

#ifndef Q_MOC_RUN
namespace TextEditor {
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

    void setContent(const TextEditor::TipContent &content);
    const TextEditor::TipContent &content() const;

    virtual void configure() = 0;
    virtual bool handleContentReplacement(const TextEditor::TipContent &content) const = 0;

private:
    TextEditor::TipContent *m_tipContent;
};

class ColorTip : public QTipLabel
{
    Q_OBJECT
public:
    ColorTip(QWidget *parent);
    virtual ~ColorTip();

    virtual void configure();
    virtual bool handleContentReplacement(const TipContent &content) const;

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

    virtual void configure();
    virtual bool handleContentReplacement(const TipContent &content) const;

private:
    virtual void paintEvent(QPaintEvent *event);
    virtual void resizeEvent(QResizeEvent *event);
};

#ifndef Q_MOC_RUN
} // namespace Internal
} // namespace TextEditor
#endif

#endif // TIPS_H
