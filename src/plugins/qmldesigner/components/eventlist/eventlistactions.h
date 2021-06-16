/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Event List module.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
******************************************************************************/

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
