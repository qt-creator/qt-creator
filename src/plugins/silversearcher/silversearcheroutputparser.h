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

    Utils::SearchResultItems parse();

private:
    int parseMatches(int lineNumber);
    int parseMatchLength();
    int parseMatchIndex();
    int parseLineNumber();
    bool parseFilePath();
    bool parseText();

    QString output;
    QRegularExpression regexp;
    bool hasRegexp = false;
    int outputSize = 0;
    int index = 0;
    Utils::SearchResultItem item;
    Utils::SearchResultItems items;
};

} // namespace SilverSearcher
