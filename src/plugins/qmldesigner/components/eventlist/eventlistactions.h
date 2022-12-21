// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <modelnodecontextmenu_helper.h>

namespace QmlDesigner {

class EventListAction : public ModelNodeAction
{
public:
    EventListAction();
};

class AssignEventEditorAction : public ModelNodeAction
{
public:
    AssignEventEditorAction();
};

class ConnectSignalAction : public ModelNodeContextMenuAction
{
public:
    ConnectSignalAction();
    TargetView targetView() const override;
};

} // namespace QmlDesigner
