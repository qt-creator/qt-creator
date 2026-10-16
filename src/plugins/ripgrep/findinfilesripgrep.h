// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/basefilefind.h>

#include <utils/searchresultitem.h>

#include <QFuture>

namespace Ripgrep {

void setupFindInFilesRipgrep();

#ifdef WITH_TESTS
TextEditor::SearchEngine *ripgrepSearchEngine();
#endif

// Runs ripgrep for \a parameters, with the optional \a searchOptions added to its arguments.
QFuture<Utils::SearchResultItems> findWithRipgrep(const TextEditor::FileFindParameters &parameters,
                                                  const QString &searchOptions = {});

} // namespace Ripgrep
