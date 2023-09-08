// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QHash>

namespace Core {

class IEditorFactory;

namespace Internal {

QHash<QString, IEditorFactory *> userPreferredEditorTypes();
void setUserPreferredEditorTypes(const QHash<QString, IEditorFactory *> &factories);

} // Internal
} // Core
