// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmt/infrastructure/uid.h"

#include <QString>

namespace qmt {

class DVisitor;
class DConstVisitor;

class QMT_EXPORT DElement
{
public:
    DElement();
    virtual ~DElement();

    Uid uid() const { return m_uid; }
    void setUid(const Uid &uid);
    void renewUid();
    virtual Uid modelUid() const = 0;

    virtual void accept(DVisitor *visitor);
    virtual void accept(DConstVisitor *visitor) const;

private:
    Uid m_uid;
};

} // namespace qmt
