// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "compilerexplorersettings.h"

#include <QDialog>

namespace CompilerExplorer {

class CompilerExplorerOptions : public QDialog
{
public:
    CompilerExplorerOptions(CompilerSettings &settings, QWidget *parent = nullptr);
};

} // namespace CompilerExplorer
