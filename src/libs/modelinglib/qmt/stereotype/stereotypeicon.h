/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#include "iconshape.h"

#include <QString>
#include <QSet>
#include <QColor>

namespace qmt {

class QMT_EXPORT StereotypeIcon
{
public:
    enum Element {
        ElementAny,
        ElementPackage,
        ElementComponent,
        ElementClass,
        ElementDiagram,
        ElementItem
    };

    enum Display {
        DisplayNone,
        DisplayLabel,
        DisplayDecoration,
        DisplayIcon,
        DisplaySmart
    };

    enum SizeLock {
        LockNone,
        LockWidth,
        LockHeight,
        LockSize,
        LockRatio
    };

    enum TextAlignment {
        TextalignBelow,
        TextalignCenter,
        TextalignNone
    };

    StereotypeIcon();
    ~StereotypeIcon();

    QString id() const { return m_id; }
    void setId(const QString &id);
    QString title() const;
    void setTitle(const QString &title);
    QSet<Element> elements() const { return m_elements; }
    void setElements(const QSet<Element> &elements);
    QSet<QString> stereotypes() const { return m_stereotypes; }
    void setStereotypes(const QSet<QString> &stereotypes);
    qreal width() const { return m_width; }
    void setWidth(qreal width);
    qreal height() const { return m_height; }
    void setHeight(qreal height);
    bool hasMinWidth() const { return m_minWidth > 0; }
    qreal minWidth() const { return m_minWidth; }
    void setMinWidth(qreal minWidth);
    bool hasMinHeight() const { return m_minHeight > 0; }
    qreal minHeight() const { return m_minHeight; }
    void setMinHeight(qreal minHeight);
    SizeLock sizeLock() const { return m_sizeLock; }
    void setSizeLock(SizeLock sizeLock);
    Display display() const { return m_display; }
    void setDisplay(Display display);
    TextAlignment textAlignment() const { return m_textAlignment; }
    void setTextAlignment(TextAlignment textAlignment);
    QColor baseColor() const { return m_baseColor; }
    void setBaseColor(const QColor &baseColor);
    IconShape iconShape() const { return m_iconShape; }
    void setIconShape(const IconShape &iconShape);

private:
    QString m_id;
    QString m_title;
    QSet<Element> m_elements;
    QSet<QString> m_stereotypes;
    qreal m_width;
    qreal m_height;
    qreal m_minWidth;
    qreal m_minHeight;
    SizeLock m_sizeLock;
    Display m_display;
    TextAlignment m_textAlignment;
    QColor m_baseColor;
    IconShape m_iconShape;
};

} // namespace qmt
