// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "abstractcustomtool.h"

#include "formeditorview.h"

namespace QmlDesigner {

using FormEditorTracing::category;

AbstractCustomTool::AbstractCustomTool()
    : AbstractFormEditorTool(nullptr)
{
    NanotraceHR::Tracer tracer{"abstract custom tool constructor", category()};
}

void AbstractCustomTool::selectedItemsChanged(const QList<FormEditorItem *> & /*itemList*/)
{
    NanotraceHR::Tracer tracer{"abstract custom tool selected items changed", category()};

    view()->changeToSelectionTool();
}

void AbstractCustomTool::focusLost()
{
}


} // namespace QmlDesigner
