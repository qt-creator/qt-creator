// Copyright (C) 2018 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>
#include <QStringList>

namespace qmt {

class ModelTreeViewData
{
public:

    bool showRelations() const { return m_showRelations; }
    void setShowRelations(bool newShowRelations);

    bool showDiagramElements() const { return m_showDiagramElements; }
    void setShowDiagramElements(bool newShowDiagramElements);

private:
    bool m_showRelations = true;
    bool m_showDiagramElements = false;
};

class ModelTreeFilterData
{
public:
    enum class Type {
        Any,
        Package,
        Component,
        Class,
        Diagram,
        Item,
        Dependency,
        Association,
        Inheritance,
        Connection
    };

    enum class Direction {
        Any,
        Outgoing,
        Incoming,
        Bidirectional
    };

    Type type() const { return m_type; }
    void setType(Type newType);

    QString customId() const { return m_customId; }
    void setCustomId(const QString &newCustomId);

    QStringList stereotypes() const { return m_stereotypes; }
    void setStereotypes(const QStringList &newStereotypes);

    QString name() const { return m_name; }
    void setName(const QString &newName);

    Direction direction() const { return m_direction; }
    void setDirection(Direction newDirection);

private:
    Type m_type = Type::Any;
    QString m_customId;
    QStringList m_stereotypes;
    QString m_name;
    Direction m_direction = Direction::Any;
};

} // namespace qmt
