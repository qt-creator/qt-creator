// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

namespace CMakeLang {

// What to put in place of the span the position and the length name.
class Edit
{
public:
    int position = 0;
    int length = 0;
    QString text;
};

} // namespace CMakeLang
