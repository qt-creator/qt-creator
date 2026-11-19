// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "jsextension.h"

#include <coreplugin/jsbind.h>

#include <qmt/controller/namecontroller.h>

#include <utils/filepath.h>

using Utils::FilePath;

namespace ModelEditor::Internal {

void registerModelingFunctions(Core::JsBind::Registrar &registrar)
{
    registrar.set("fileNameToElementName", [](FilePath file) {
        return qmt::NameController::convertFileNameToElementName(file);
    });
    registrar.set("elementNameToFileName", [](QString element) {
        return qmt::NameController::convertElementNameToBaseFileName(element);
    });
}

} // namespace ModelEditor::Internal
