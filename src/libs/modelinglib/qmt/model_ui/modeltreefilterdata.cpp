// Copyright (C) 2018 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "modeltreefilterdata.h"

namespace qmt {

void ModelTreeViewData::setShowRelations(bool newShowRelations)
{
    m_showRelations = newShowRelations;
}

void ModelTreeViewData::setShowDiagramElements(bool newShowDiagramElements)
{
    m_showDiagramElements = newShowDiagramElements;
}

void ModelTreeFilterData::setType(Type newType)
{
    m_type = newType;
}

void ModelTreeFilterData::setCustomId(const QString &newCustomId)
{
    m_customId = newCustomId;
}

void ModelTreeFilterData::setStereotypes(const QStringList &newStereotypes)
{
    m_stereotypes = newStereotypes;
}

void ModelTreeFilterData::setName(const QString &newName)
{
    m_name = newName;
}

void ModelTreeFilterData::setDirection(Direction newDirection)
{
    m_direction = newDirection;
}

} // namespace qmt
