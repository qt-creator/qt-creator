// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmljseditor/qmljseditor_global.h>

namespace TextEditor { class BaseHoverHandler; }

namespace QmlJSEditor {

QMLJSEDITOR_EXPORT TextEditor::BaseHoverHandler &qmlJSHoverHandler();

} // namespace QmlJSEditor
