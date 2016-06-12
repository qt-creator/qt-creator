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
#include <QList>
#include <QSet>
#include <QColor>

namespace qmt {

class QMT_EXPORT CustomRelation
{
public:
    enum class Element {
        Relation,
        Dependency,
        Inheritance,
        Association
    };

    enum class Direction {
        AtoB,
        BToA,
        Bi
    };

    enum class Relationship {
        Association,
        Aggregation,
        Composition
    };

    enum class ShaftPattern {
        Solid,
        Dash,
        Dot,
        DashDot,
        DashDotDot
    };

    enum class Head {
        None,
        Shape,
        Arrow,
        Triangle,
        FilledTriangle,
        Diamond,
        FilledDiamond
    };

    enum class ColorType {
        EndA,
        EndB,
        Custom
    };

    class End {
    public:
        QList<QString> endItems() const { return m_endItems; }
        void setEndItems(const QList<QString> &endItems);
        QString role() const { return m_role; }
        void setRole(const QString &role);
        QString cardinality() const { return m_cardinality; }
        void setCardinality(const QString &cardinality);
        bool navigable() const { return m_navigable; }
        void setNavigable(bool navigable);
        Relationship relationship() const { return m_relationship; }
        void setRelationship(Relationship relationship);
        Head head() const { return m_head; }
        void setHead(Head head);
        IconShape shape() const { return m_shape; }
        void setShape(const IconShape &shape);

    private:
        QList<QString> m_endItems;
        QString m_role;
        QString m_cardinality;
        bool m_navigable = false;
        Relationship m_relationship = Relationship::Association;
        Head m_head = Head::None;
        IconShape m_shape;
    };

    CustomRelation();
    ~CustomRelation();

    bool isNull() const;
    Element element() const { return m_element; }
    void setElement(Element element);
    QString id() const { return m_id; }
    void setId(const QString &id);
    QString title() const { return m_title; }
    void setTitle(const QString &title);
    QList<QString> endItems() const { return m_endItems; }
    void setEndItems(const QList<QString> &endItems);
    QSet<QString> stereotypes() const { return m_stereotypes; }
    void setStereotypes(const QSet<QString> &stereotypes);
    QString name() const { return m_name; }
    void setName(const QString &name);
    Direction direction() const { return m_direction; }
    void setDirection(Direction direction);
    End endA() const { return m_endA; }
    void setEndA(const End &end);
    End endB() const { return m_endB; }
    void setEndB(const End &end);
    ShaftPattern shaftPattern() const { return m_shaftPattern; }
    void setShaftPattern(ShaftPattern shaftPattern);
    ColorType colorType() const { return m_colorType; }
    void setColorType(ColorType colorType);
    QColor color() const { return m_color; }
    void setColor(const QColor &color);

private:
    Element m_element = Element::Relation;
    QString m_id;
    QString m_title;
    QList<QString> m_endItems;
    QSet<QString> m_stereotypes;
    QString m_name;
    Direction m_direction = Direction::AtoB;
    End m_endA;
    End m_endB;
    ShaftPattern m_shaftPattern = ShaftPattern::Solid;
    ColorType m_colorType = ColorType::EndA;
    QColor m_color;
};

inline uint qHash(CustomRelation::Relationship relationship) {
    return ::qHash(static_cast<int>(relationship));
}

inline uint qHash(CustomRelation::ShaftPattern pattern) {
    return ::qHash(static_cast<int>(pattern));
}

inline uint qHash(CustomRelation::Head head) {
    return ::qHash(static_cast<int>(head));
}

} // namespace qmt
