// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QStringList>

namespace Core { class IDocument; }
namespace ProjectExplorer { class Project; }

namespace ClangTools::Internal {

void offerToIgnoreCompilerFlags(ProjectExplorer::Project *project, const QStringList &flags);

void offerToIgnoreCompilerFlags(Core::IDocument *document, ProjectExplorer::Project *project,
                                const QStringList &flags);

} // namespace ClangTools::Internal
