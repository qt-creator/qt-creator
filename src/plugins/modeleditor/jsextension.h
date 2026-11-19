// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace Core::JsBind { class Registrar; }

namespace ModelEditor::Internal {

// Registers the functions of the "Modeling" JavaScript object.
void registerModelingFunctions(Core::JsBind::Registrar &registrar);

} // namespace ModelEditor::Internal
