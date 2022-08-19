// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "componentcore_constants.h"
#include "selectioncontext.h"

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT ActionInterface
{
public:
    enum Type {
        ContextMenu,
        ContextMenuAction,
        ToolBarAction,
        Action,
        FormEditorAction,
        Edit3DAction
    };

    enum Priorities {
        HighestPriority = ComponentCoreConstants::priorityFirst,
        CustomActionsPriority = ComponentCoreConstants::priorityCustomActions,
        RefactoringActionsPriority = ComponentCoreConstants::priorityRefactoring,
        LowestPriority = ComponentCoreConstants::priorityLast
    };

    enum class TargetView {
        Undefined,
        ConnectionEditor
    };

    virtual ~ActionInterface() = default;

    virtual QAction *action() const = 0;
    virtual QByteArray category() const = 0;
    virtual QByteArray menuId() const = 0;
    virtual int priority() const = 0;
    virtual Type type() const = 0;
    virtual void currentContextChanged(const SelectionContext &selectionState) = 0;
    virtual TargetView targetView() const { return TargetView::Undefined; }

};

} //QmlDesigner
