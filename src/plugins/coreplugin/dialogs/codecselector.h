// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../core_global.h"

#include <utils/textcodec.h>

namespace Core {

struct CORE_EXPORT CodecSelectorResult
{
    enum Action { Cancel, Reload, Save };
    Action action;
    Utils::TextEncoding encoding;
};

CORE_EXPORT CodecSelectorResult askForCodec(class BaseTextDocument *doc);

} // namespace Core
