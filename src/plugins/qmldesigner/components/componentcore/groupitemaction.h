// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <modelnodecontextmenu_helper.h>

namespace QmlDesigner {

class GroupItemAction: public ModelNodeAction
{
public:
    GroupItemAction(const QIcon &icon,
                    const QKeySequence &key,
                    int priority);


protected:
    virtual void updateContext() override;
    virtual bool isChecked(const SelectionContext &selectionContext) const override;
};

}
