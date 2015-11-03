/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMT_STEREOTYPEICON_H
#define QMT_STEREOTYPEICON_H

#include "iconshape.h"

#include <QString>
#include <QSet>
#include <QColor>


namespace qmt {

class QMT_EXPORT StereotypeIcon
{
public:

    enum Element {
        ELEMENT_ANY,
        ELEMENT_PACKAGE,
        ELEMENT_COMPONENT,
        ELEMENT_CLASS,
        ELEMENT_DIAGRAM,
        ELEMENT_ITEM
    };

    enum Display {
        DISPLAY_NONE,
        DISPLAY_LABEL,
        DISPLAY_DECORATION,
        DISPLAY_ICON,
        DISPLAY_SMART
    };

    enum SizeLock {
        LOCK_NONE,
        LOCK_WIDTH,
        LOCK_HEIGHT,
        LOCK_SIZE,
        LOCK_RATIO
    };

    enum TextAlignment {
        TEXTALIGN_BELOW,
        TEXTALIGN_CENTER,
        TEXTALIGN_NONE
    };

public:

    StereotypeIcon();

    ~StereotypeIcon();

public:

    QString getId() const { return m_id; }

    void setId(const QString &id);

    QString getTitle() const;

    void setTitle(const QString &title);

    QSet<Element> getElements() const { return m_elements; }

    void setElements(const QSet<Element> &elements);

    QSet<QString> getStereotypes() const { return m_stereotypes; }

    void setStereotypes(const QSet<QString> &stereotypes);

    qreal getWidth() const { return m_width; }

    void setWidth(qreal width);

    qreal getHeight() const { return m_height; }

    void setHeight(qreal height);

    bool hasMinWidth() const { return m_minWidth > 0; }

    qreal getMinWidth() const { return m_minWidth; }

    void setMinWidth(qreal min_width);

    bool hasMinHeight() const { return m_minHeight > 0; }

    qreal getMinHeight() const { return m_minHeight; }

    void setMinHeight(qreal min_height);

    SizeLock getSizeLock() const { return m_sizeLock; }

    void setSizeLock(SizeLock size_lock);

    Display getDisplay() const { return m_display; }

    void setDisplay(Display display);

    TextAlignment getTextAlignment() const { return m_textAlignment; }

    void setTextAlignment(TextAlignment text_alignment);

    QColor getBaseColor() const { return m_baseColor; }

    void setBaseColor(const QColor &base_color);

    IconShape getIconShape() const { return m_iconShape; }

    void setIconShape(const IconShape &icon_shape);

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

#endif // QMT_STEREOTYPEICON_H
