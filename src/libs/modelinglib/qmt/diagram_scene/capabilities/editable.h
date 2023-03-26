// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace qmt {

class IEditable
{
public:
    virtual ~IEditable() { }

    virtual bool isEditable() const = 0;
    virtual bool isEditing() const = 0;

    virtual void edit() = 0;
    virtual void finishEdit() = 0;
};

} // namespace qmt
