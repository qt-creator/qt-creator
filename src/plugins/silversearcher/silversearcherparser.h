// Copyright (C) 2017 Przemyslaw Gorszkowski <pgorszkowski@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/searchresultitem.h>

#include <QFuture>
#include <QRegularExpression>

namespace Utils { class FilePath; }

namespace SilverSearcher {

Utils::SearchResultItems parse(const QFuture<void> &future, const QString &input,
                               const std::optional<QRegularExpression> &regExp,
                               Utils::FilePath *lastFilePath);

Utils::SearchResultItems parse(const QString &input,
                               const std::optional<QRegularExpression> &regExp = {});

} // namespace SilverSearcher
