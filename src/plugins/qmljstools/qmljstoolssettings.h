// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmljstools_global.h"

#include "qmljscodestylesettings.h"

namespace QmlJSTools {

QMLJSTOOLS_EXPORT QmlJSCodeStylePreferences *globalQmlJSCodeStyle();

namespace Internal { void setupQmlJSToolsSettings(); }

} // namespace QmlJSTools
