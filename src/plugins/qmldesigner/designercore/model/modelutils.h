// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmldesignercorelib_global.h"

#include <import.h>
#include <model.h>

#include <functional>

namespace QmlDesigner::Utils {

QMLDESIGNERCORE_EXPORT bool addImportsWithCheck(const QStringList &importNames,
                                                const std::function<bool(const Import &)> &predicate,
                                                Model *model);
QMLDESIGNERCORE_EXPORT bool addImportsWithCheck(const QStringList &importNames, Model *model);
QMLDESIGNERCORE_EXPORT bool addImportWithCheck(const QString &importName,
                                               const std::function<bool(const Import &)> &predicate,
                                               Model *model);
QMLDESIGNERCORE_EXPORT bool addImportWithCheck(const QString &importName, Model *model);

} // namespace QmlDesigner::Utils
