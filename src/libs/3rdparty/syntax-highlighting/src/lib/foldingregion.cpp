/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: MIT
*/

#include "foldingregion.h"

using namespace KSyntaxHighlighting;

static_assert(sizeof(FoldingRegion) == sizeof(int), "FoldingRegion is size-sensitive to frequent use in KTextEditor!");

FoldingRegion::FoldingRegion() = default;

FoldingRegion::FoldingRegion(Type type, int id)
    : m_idWithType((type == End) ? -id : id)
{
}

bool FoldingRegion::operator==(const FoldingRegion &other) const
{
    return m_idWithType == other.m_idWithType;
}

bool FoldingRegion::isValid() const
{
    return m_idWithType != 0;
}

int FoldingRegion::id() const
{
    return (m_idWithType < 0) ? -m_idWithType : m_idWithType;
}

FoldingRegion::Type FoldingRegion::type() const
{
    if (isValid()) {
        return (m_idWithType < 0) ? End : Begin;
    }
    return None;
}

FoldingRegion FoldingRegion::sibling() const
{
    return isValid() ? FoldingRegion(type() ? End : Begin, id()) : FoldingRegion();
}
