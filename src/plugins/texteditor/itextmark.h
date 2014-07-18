/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef ITEXTMARK_H
#define ITEXTMARK_H

#include "texteditor_global.h"

#include <QObject>
#include <QIcon>

QT_BEGIN_NAMESPACE
class QPainter;
class QRect;
class QTextBlock;
QT_END_NAMESPACE

namespace TextEditor {

class ITextEditor;
class BaseTextDocument;

class TEXTEDITOR_EXPORT ITextMark
{
public:
    ITextMark(int line)
        : m_baseTextDocument(0),
          m_lineNumber(line),
          m_priority(NormalPriority),
          m_visible(true),
          m_widthFactor(1.0)
    {}
    virtual ~ITextMark();

    // determine order on markers on the same line.
    enum Priority
    {
        LowPriority,
        NormalPriority,
        HighPriority // shown on top.
    };

    int lineNumber() const;
    virtual void paint(QPainter *painter, const QRect &rect) const;
    virtual void updateLineNumber(int lineNumber);
    virtual void updateBlock(const QTextBlock &block);
    virtual void move(int line);
    virtual void removedFromEditor();
    virtual bool isClickable() const;
    virtual void clicked();
    virtual bool isDraggable() const;
    virtual void dragToLine(int lineNumber);

    void setIcon(const QIcon &icon);
    // call this if the icon has changed.
    void updateMarker();
    Priority priority() const;
    void setPriority(Priority prioriy);
    bool isVisible() const;
    void setVisible(bool isVisible);
    double widthFactor() const;
    void setWidthFactor(double factor);

    BaseTextDocument *baseTextDocument() const;
    void setBaseTextDocument(BaseTextDocument *baseTextDocument);

private:
    Q_DISABLE_COPY(ITextMark)
    BaseTextDocument *m_baseTextDocument;
    int m_lineNumber;
    Priority m_priority;
    bool m_visible;
    QIcon m_icon;
    double m_widthFactor;
};

} // namespace TextEditor

#endif // ITEXTMARK_H
