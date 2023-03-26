// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QIODevice>

namespace QmlProfiler {
namespace Internal {

void fakeDebugServer(QIODevice *socket);

} // namespace Internal
} // namespace QmlProfiler
