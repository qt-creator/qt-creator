// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/basefilewizard.h>
#include <coreplugin/basefilewizardfactory.h>
#include <utils/fileutils.h>
#include <utils/wizard.h>

namespace Utils { class FileWizardPage; }

namespace GenericProjectManager {
namespace Internal {

class FilesSelectionWizardPage;

void setupGenericProjectWizard();

} // namespace Internal
} // namespace GenericProjectManager
