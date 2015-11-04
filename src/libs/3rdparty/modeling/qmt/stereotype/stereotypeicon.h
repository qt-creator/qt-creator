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

public:

    StereotypeIcon();

    ~StereotypeIcon();

public:

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

#endif // QMT_STEREOTYPEICON_H
