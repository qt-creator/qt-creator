// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "msourceexpansion.h"

#include "qmt/infrastructure/qmtassert.h"

namespace qmt {

MSourceExpansion::MSourceExpansion()
{
}

MSourceExpansion::MSourceExpansion(const MSourceExpansion &rhs)
    : MExpansion(rhs),
      m_sourceId(rhs.m_sourceId),
      m_isTransient(rhs.m_isTransient)
{
}

MSourceExpansion::~MSourceExpansion()
{
}

MSourceExpansion &MSourceExpansion::operator=(const MSourceExpansion &rhs)
{
    if (this != &rhs) {
        m_sourceId = rhs.m_sourceId;
        m_isTransient = rhs.m_isTransient;
    }
    return *this;
}

MSourceExpansion *MSourceExpansion::clone(const MElement &rhs) const
{
    auto rightExpansion = dynamic_cast<MSourceExpansion *>(rhs.expansion());
    QMT_ASSERT(rightExpansion, return nullptr);
    auto expansion = new MSourceExpansion(*rightExpansion);
    return expansion;
}

void MSourceExpansion::setSourceId(const QString &sourceId)
{
    m_sourceId = sourceId;
}

void MSourceExpansion::setTransient(bool transient)
{
    m_isTransient = transient;
}

} // namespace qmt
