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

#ifndef ITEXTMARK_H
#define ITEXTMARK_H

#include "texteditor_global.h"

#include <QObject>
#include <QList>
#include <QMap>
#include <QIcon>

QT_BEGIN_NAMESPACE
class QIcon;
class QPainter;
class QRect;
class QTextBlock;
QT_END_NAMESPACE

namespace TextEditor {

class ITextEditor;
class ITextMarkable;

class TEXTEDITOR_EXPORT ITextMark
{
public:
    ITextMark(int line)
        : m_markableInterface(0),
          m_lineNumber(line),
          m_priority(NormalPriority),
          m_widthFactor(1.0),
          m_visible(true)
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

    ITextMarkable *markableInterface() const;
    void setMarkableInterface(ITextMarkable *markableInterface);
private:
    Q_DISABLE_COPY(ITextMark)
    ITextMarkable *m_markableInterface;
    int m_lineNumber;
    QIcon m_icon;
    Priority m_priority;
    double m_widthFactor;
    bool m_visible;
};

typedef QList<ITextMark *> TextMarks;


class TEXTEDITOR_EXPORT ITextMarkable : public QObject
{
    Q_OBJECT
public:
    ITextMarkable(QObject *parent = 0) : QObject(parent) {}

    virtual TextMarks marks() const = 0;
    virtual bool addMark(ITextMark *mark) = 0;
    virtual TextMarks marksAt(int line) const = 0;
    virtual void removeMark(ITextMark *mark) = 0;
    virtual void updateMark(ITextMark *mark) = 0;
};

} // namespace TextEditor

#endif // ITEXTMARK_H
