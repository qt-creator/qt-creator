// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/searchresultitem.h>

#include <QRegularExpression>

#include <optional>

QT_BEGIN_NAMESPACE
template <typename T>
class QFuture;
QT_END_NAMESPACE

namespace Ripgrep {

// Parses the JSON lines of "rg --json". The output arrives in chunks that do not end at the end
// of a line, so what follows the last line break of \a input is kept in \a remainder for the
// next call. The captured texts of a match are only reported for a regular expression search.
Utils::SearchResultItems parse(const QFuture<void> &future, const QString &input,
                               const std::optional<QRegularExpression> &regExp,
                               QString *remainder);

Utils::SearchResultItems parse(const QString &input,
                               const std::optional<QRegularExpression> &regExp = {});

} // namespace Ripgrep
