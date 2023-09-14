// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

namespace QmlDesigner::CoreUtils {

template<typename Element, typename... Elements>
#ifdef Q_CC_MSVC
__forceinline
#else
[[gnu::always_inline]]
#endif
    constexpr bool
    contains(Element element, Elements... elements)
{
    return ((element == elements) || ...);
}

} // namespace QmlDesigner::CoreUtils
