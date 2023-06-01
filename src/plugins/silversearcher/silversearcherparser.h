// Copyright (C) 2017 Przemyslaw Gorszkowski <pgorszkowski@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/searchresultitem.h>

#include <QPromise>
#include <QRegularExpression>

namespace Utils { class FilePath; }

namespace SilverSearcher {

class ParserState
{
public:
    Utils::FilePath m_lastFilePath;
    int m_reportedResultsCount = 0;
};

void parse(QPromise<Utils::SearchResultItems> &promise, const QString &input,
           ParserState *parserState = nullptr,
           const std::optional<QRegularExpression> &regExp = {});

Utils::SearchResultItems parse(const QString &input,
                               const std::optional<QRegularExpression> &regExp = {});

} // namespace SilverSearcher
