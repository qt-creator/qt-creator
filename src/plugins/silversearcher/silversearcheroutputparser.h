// Copyright (C) 2017 Przemyslaw Gorszkowski <pgorszkowski@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/find/searchresultwindow.h>
#include <utils/filesearch.h>

#include <QList>
#include <QRegularExpression>

namespace SilverSearcher {

class SilverSearcherOutputParser
{
public:
    SilverSearcherOutputParser(const QString &output, const QRegularExpression &regexp = {});

    Utils::FileSearchResultList parse();

private:
    int parseMatches();
    bool parseMatchLength();
    bool parseMatchIndex();
    bool parseLineNumber();
    bool parseFilePath();
    bool parseText();

    QString output;
    QRegularExpression regexp;
    bool hasRegexp = false;
    int outputSize = 0;
    int index = 0;
    Utils::FileSearchResult item;
    Utils::FileSearchResultList items;
};

} // namespace SilverSearcher
