// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "compilerexploreroptions.h"

#include "compilerexplorersettings.h"

#include <utils/layoutbuilder.h>

#include <QFutureWatcher>
#include <QScrollArea>
#include <QStandardItemModel>

namespace CompilerExplorer {

CompilerExplorerOptions::CompilerExplorerOptions(CompilerSettings &compilerSettings, QWidget *parent)
    : QDialog(parent, Qt::Popup)
{
    using namespace Layouting;

    // clang-format off
    Form {
        compilerSettings.compiler, br,
        compilerSettings.compilerOptions, br,
        compilerSettings.libraries, br,
        compilerSettings.compileToBinaryObject, br,
        compilerSettings.executeCode, br,
        compilerSettings.intelAsmSyntax, br,
        compilerSettings.demangleIdentifiers, br,
    }.attachTo(this);
    // clang-format on
}

} // namespace CompilerExplorer
