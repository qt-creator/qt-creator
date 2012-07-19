/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

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
    virtual bool clickable() const;
    virtual void clicked();

    void setIcon(const QIcon &icon);
    // call this if the icon has changed.
    void updateMarker();
    Priority priority() const;
    void setPriority(Priority prioriy);
    bool visible() const;
    void setVisible(bool visible);
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
