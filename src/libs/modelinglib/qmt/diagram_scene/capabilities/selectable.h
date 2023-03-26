// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QRectF>

namespace qmt {

class ISelectable
{
public:
    virtual ~ISelectable() { }

    virtual bool isSecondarySelected() const = 0;
    virtual void setSecondarySelected(bool secondarySelected) = 0;
    virtual bool isFocusSelected() const = 0;
    virtual void setFocusSelected(bool focusSelected) = 0;
    virtual QRectF getSecondarySelectionBoundary() = 0;
    virtual void setBoundarySelected(const QRectF &boundary, bool secondary) = 0;
};

} // namespace qmt
