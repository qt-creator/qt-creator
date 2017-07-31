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
