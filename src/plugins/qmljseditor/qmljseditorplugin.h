// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace QmlJS { class JsonSchemaManager; }

namespace QmlJSEditor::Internal {

class QmlJSQuickFixAssistProvider;

QmlJSQuickFixAssistProvider *quickFixAssistProvider();
QmlJS::JsonSchemaManager *jsonManager();

} // QmlJSEditor::Internal
