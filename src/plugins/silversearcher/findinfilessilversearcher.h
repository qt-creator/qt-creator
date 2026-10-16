// Copyright (C) 2017 Przemyslaw Gorszkowski <pgorszkowski@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/basefilefind.h>

#include <utils/searchresultitem.h>

#include <QFuture>

namespace SilverSearcher {

void setupFindInFilesSilverSearcher();

// The arguments of ag for \a parameters, with the optional \a searchOptions added to them.
QStringList silverSearcherArguments(const TextEditor::FileFindParameters &parameters,
                                    const QString &searchOptions = {});

// Runs ag for \a parameters, with the optional \a searchOptions added to its arguments.
QFuture<Utils::SearchResultItems> findWithSilverSearcher(
    const TextEditor::FileFindParameters &parameters, const QString &searchOptions = {});

} // namespace SilverSearcher
