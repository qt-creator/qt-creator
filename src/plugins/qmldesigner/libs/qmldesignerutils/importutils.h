// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmldesignerutils_global.h"

#include <optional>
#include <QStringView>

namespace QmlDesigner::ImportUtils {

QMLDESIGNERUTILS_EXPORT std::optional<QStringView::iterator> find_import_location(QStringView text,
                                                                                  QStringView directory);

} // namespace QmlDesigner::ImportUtils
