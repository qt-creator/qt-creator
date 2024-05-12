// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "jsextension.h"

#include <qmt/controller/namecontroller.h>

using Utils::FilePath;

QString ModelEditor::Internal::JsExtension::fileNameToElementName(const FilePath &file)
{
    return qmt::NameController::convertFileNameToElementName(file);
}

QString ModelEditor::Internal::JsExtension::elementNameToFileName(const QString &element)
{
    return qmt::NameController::convertElementNameToBaseFileName(element);
}
