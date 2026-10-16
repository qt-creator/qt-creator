// Copyright (C) 2017 Przemyslaw Gorszkowski <pgorszkowski@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>
#include <utils/searchresultitem.h>

#include <QFuture>
#include <QRegularExpression>
#include <optional>

namespace SilverSearcher {

struct OutputLine
{
    int lineNumber = -1;
    QList<QPair<int, int>> hits; // <hit start pos, hit length> in bytes
    QByteArray bytes; // The text as ag printed it
    QString text;
    bool isContext = false;
};

// A line that the column of a hit can count from, because it does not start inside a match
struct Start
{
    int lineNumber = -1;
    int droppedBytes = 0; // Size of the lines from this one on that are no longer in the run
};

// The output arrives in chunks that do not end at the end of a line, and a match across lines is
// reported on its last line, after the lines before it. This is what is kept between the chunks.
class ParserState
{
public:
    Utils::FilePath filePath;
    QString remainder; // The beginning of a line that was not received completely
    // The latest lines of the file with consecutive numbers, from the last line with hits on
    QList<OutputLine> run;
    QList<Start> starts; // Lines of the run, or that were before it, that hits can count from
    // The text searched for in a search that is not for a regular expression, if known. It is
    // used to tell how a match across lines was counted.
    std::optional<QString> plainText;
    Qt::CaseSensitivity plainTextCaseSensitivity = Qt::CaseSensitive;
    QRegularExpression wholeMatchSource; // The regular expression that wholeMatch was built from
    std::optional<QRegularExpression> wholeMatch; // The regular expression, anchored
};

Utils::SearchResultItems parse(const QFuture<void> &future, const QString &input,
                               const std::optional<QRegularExpression> &regExp,
                               ParserState *state);

Utils::SearchResultItems parse(const QString &input,
                               const std::optional<QRegularExpression> &regExp = {});

} // namespace SilverSearcher
