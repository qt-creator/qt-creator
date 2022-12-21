// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

class CGI
{
public:
    static QString encodeURL(const QString &rawText);
    static QString decodeURL(const QString &urlText);

    enum HTMLconvertFlags {
        Normal,
        LineBreaks = 0x0001,
        Spaces     = 0x0002,
        Tabs       = 0x0004
    };

    static QString encodeHTML(const QString &rawText, int conversionFlags = 0);

private:
    inline short charToHex(const QChar &ch);
    inline QChar hexToChar(const QString &hx);
};
