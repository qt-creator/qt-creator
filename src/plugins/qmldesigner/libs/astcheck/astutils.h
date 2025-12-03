// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QStringView>

namespace QmlDesigner::AstUtils {

bool isQmlKeyword(QStringView id);
bool isDiscouragedQmlId(QStringView id);
bool isQmlBuiltinType(QStringView id);
bool isBannedQmlId(QStringView id);

} // namespace QmlDesigner::AstUtils
