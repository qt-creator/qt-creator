/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: MIT
*/

#include "foldingregion.h"

using namespace KSyntaxHighlighting;

static_assert(sizeof(FoldingRegion) == 2, "FoldingRegion is size-sensitive to frequent use in KTextEditor!");

FoldingRegion::FoldingRegion()
    : m_type(None)
    , m_id(0)
{
}

FoldingRegion::FoldingRegion(Type type, quint16 id)
    : m_type(type)
    , m_id(id)
{
}

bool FoldingRegion::operator==(const FoldingRegion &other) const
{
    return m_id == other.m_id && m_type == other.m_type;
}

bool FoldingRegion::isValid() const
{
    return type() != None;
}

quint16 FoldingRegion::id() const
{
    return m_id;
}

FoldingRegion::Type FoldingRegion::type() const
{
    return static_cast<FoldingRegion::Type>(m_type);
}
