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

#include "texteditor_global.h"

#include <coreplugin/id.h>
#include <utils/theme/theme.h>

#include <QIcon>

QT_BEGIN_NAMESPACE
class QGridLayout;
class QLayout;
class QPainter;
class QRect;
class QTextBlock;
QT_END_NAMESPACE

namespace TextEditor {

class BaseTextEditor;
class TextDocument;

namespace Internal { class TextMarkRegistry; }

class TEXTEDITOR_EXPORT TextMark
{
public:
    TextMark(const QString &fileName, int lineNumber, Core::Id category, double widthFactor = 1.0);
    virtual ~TextMark();

    // determine order on markers on the same line.
    enum Priority
    {
        LowPriority,
        NormalPriority,
        HighPriority // shown on top.
    };

    QString fileName() const;
    int lineNumber() const;

    virtual void paint(QPainter *painter, const QRect &rect) const;
    /// called if the filename of the document changed
    virtual void updateFileName(const QString &fileName);
    virtual void updateLineNumber(int lineNumber);
    virtual void updateBlock(const QTextBlock &block);
    virtual void move(int line);
    virtual void removedFromEditor();
    virtual bool isClickable() const;
    virtual void clicked();
    virtual bool isDraggable() const;
    virtual void dragToLine(int lineNumber);
    void addToToolTipLayout(QGridLayout *target);
    virtual bool addToolTipContent(QLayout *target);

    static Utils::Theme::Color categoryColor(Core::Id category);
    static bool categoryHasColor(Core::Id category);
    static void setCategoryColor(Core::Id category, Utils::Theme::Color color);
    static void setDefaultToolTip(Core::Id category, const QString &toolTip);
    void setIcon(const QIcon &icon);
    const QIcon &icon() const;
    // call this if the icon has changed.
    void updateMarker();
    Priority priority() const;
    void setPriority(Priority prioriy);
    bool isVisible() const;
    void setVisible(bool isVisible);
    Core::Id category() const;
    double widthFactor() const;
    void setWidthFactor(double factor);

    TextDocument *baseTextDocument() const;
    void setBaseTextDocument(TextDocument *baseTextDocument);

    QString toolTip() const;
    void setToolTip(const QString &toolTip);

private:
    Q_DISABLE_COPY(TextMark)
    friend class Internal::TextMarkRegistry;

    TextDocument *m_baseTextDocument;
    QString m_fileName;
    int m_lineNumber;
    Priority m_priority;
    bool m_visible;
    QIcon m_icon;
    QColor m_color;
    Core::Id m_category;
    double m_widthFactor;
    QString m_toolTip;
};

} // namespace TextEditor
