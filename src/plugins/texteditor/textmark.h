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

class TextDocument;

class TEXTEDITOR_EXPORT TextMark
{
public:
    TextMark(const QString &fileName, int lineNumber, Core::Id category, double widthFactor = 1.0);
    TextMark() = delete;
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

    virtual void paintIcon(QPainter *painter, const QRect &rect) const;
    virtual void paintAnnotation(QPainter &painter, QRectF *annotationRect,
                                 const qreal fadeInOffset, const qreal fadeOutOffset,
                                 const QPointF &contentOffset) const;
    struct AnnotationRects
    {
        QRectF fadeInRect;
        QRectF annotationRect;
        QRectF iconRect;
        QRectF textRect;
        QRectF fadeOutRect;
        QString text;
    };
    AnnotationRects annotationRects(const QRectF &boundingRect, const QFontMetrics &fm,
                                    const qreal fadeInOffset, const qreal fadeOutOffset) const;
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
    void addToToolTipLayout(QGridLayout *target) const;
    virtual bool addToolTipContent(QLayout *target) const;

    void setIcon(const QIcon &icon) { m_icon = icon; }
    const QIcon &icon() const { return m_icon; }
    // call this if the icon has changed.
    void updateMarker();
    Priority priority() const { return m_priority;}
    void setPriority(Priority prioriy);
    bool isVisible() const;
    void setVisible(bool isVisible);
    Core::Id category() const { return m_category; }
    double widthFactor() const;
    void setWidthFactor(double factor);

    Utils::Theme::Color color() const;
    void setColor(const Utils::Theme::Color &color);
    bool hasColor() const { return m_hasColor; }

    QString defaultToolTip() const { return m_defaultToolTip; }
    void setDefaultToolTip(const QString &toolTip) { m_defaultToolTip = toolTip; }

    TextDocument *baseTextDocument() const { return m_baseTextDocument; }
    void setBaseTextDocument(TextDocument *baseTextDocument) { m_baseTextDocument = baseTextDocument; }

    QString lineAnnotation() const { return m_lineAnnotation; }
    void setLineAnnotation(const QString &lineAnnotation) { m_lineAnnotation = lineAnnotation; }

    QString toolTip() const { return m_toolTip; }
    void setToolTip(const QString &toolTip) { m_toolTip = toolTip; }

private:
    Q_DISABLE_COPY(TextMark)

    TextDocument *m_baseTextDocument = nullptr;
    QString m_fileName;
    int m_lineNumber = 0;
    Priority m_priority = LowPriority;
    QIcon m_icon;
    Utils::Theme::Color m_color = Utils::Theme::TextColorNormal;
    bool m_visible = false;
    bool m_hasColor = false;
    Core::Id m_category;
    double m_widthFactor = 1.0;
    QString m_lineAnnotation;
    QString m_toolTip;
    QString m_defaultToolTip;
};

} // namespace TextEditor
