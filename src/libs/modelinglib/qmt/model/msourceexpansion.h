// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "melement.h"

namespace qmt {

class QMT_EXPORT MSourceExpansion : public MExpansion
{
public:
    MSourceExpansion();
    MSourceExpansion(const MSourceExpansion &rhs);
    ~MSourceExpansion() override;

    MSourceExpansion &operator=(const MSourceExpansion &rhs);

    MSourceExpansion *clone(const MElement &rhs) const override;

    QString sourceId() const { return m_sourceId; }
    void setSourceId(const QString &sourceId);
    bool isTransient() const { return m_isTransient; }
    void setTransient(bool transient);

private:
    QString m_sourceId;
    bool m_isTransient = false;
};

} // namespace qmt
