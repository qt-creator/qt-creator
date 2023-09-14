// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmljseditor_global.h"
#include "qmljsquickfix.h"

namespace QmlJSEditor {

class QmlJSEditorWidget;

QMLJSEDITOR_EXPORT void matchComponentFromObjectDefQuickFix(
    const Internal::QmlJSQuickFixAssistInterface *interface, QuickFixOperations &result);

QMLJSEDITOR_EXPORT void performComponentFromObjectDef(QmlJSEditorWidget *editor,
                                                      const QString &fileName,
                                                      QmlJS::AST::UiObjectDefinition *objDef,
                                                      const QString &importData);

} // namespace QmlJSEditor
