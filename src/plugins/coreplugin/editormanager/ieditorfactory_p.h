// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/mimeutils.h>

#include <QHash>

namespace Core {

class EditorType;

namespace Internal {

QHash<Utils::MimeType, EditorType *> userPreferredEditorTypes();
void setUserPreferredEditorTypes(const QHash<Utils::MimeType, EditorType *> &factories);

} // Internal
} // Core
