// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <modelnodecontextmenu_helper.h>
namespace QmlDesigner {

class ParentAnchorAction : public ModelNodeAction
{
public:
    ParentAnchorAction(const QByteArray &id,
                       const QString &description,
                       const QIcon &icon,
                       const QString &tooltip,
                       const QByteArray &category,
                       const QKeySequence &key,
                       int priority,
                       AnchorLineType lineType);

    bool isChecked(const SelectionContext &selectionState) const override;

private:
    const AnchorLineType m_lineType;
};

}
