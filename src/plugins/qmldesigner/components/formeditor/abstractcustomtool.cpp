// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "abstractcustomtool.h"

#include "formeditorview.h"

namespace QmlDesigner {

AbstractCustomTool::AbstractCustomTool()
    : AbstractFormEditorTool(nullptr)
{
}

void AbstractCustomTool::selectedItemsChanged(const QList<FormEditorItem *> & /*itemList*/)
{
    view()->changeToSelectionTool();
}

void AbstractCustomTool::focusLost()
{
}


} // namespace QmlDesigner
