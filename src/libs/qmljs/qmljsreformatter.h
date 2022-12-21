// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmljs_global.h"

#include "qmljsdocument.h"

namespace QmlJS {
QMLJS_EXPORT QString reformat(const Document::Ptr &doc);
QMLJS_EXPORT QString reformat(const Document::Ptr &doc, int indentSize, int tabSize, int lineLength);
} // namespace QmlJS
