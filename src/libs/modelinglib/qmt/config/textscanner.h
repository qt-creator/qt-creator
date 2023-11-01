// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sourcepos.h"

#include "qmt/infrastructure/exceptions.h"

#include <QObject>

namespace qmt {

class ITextSource;
class SourceChar;
class Token;

class QMT_EXPORT TextScannerError : public Exception
{
public:
    TextScannerError(const QString &errorMsg, const SourcePos &sourcePos);
    ~TextScannerError() override;

    SourcePos sourcePos() const { return m_sourcePos; }

private:
    SourcePos m_sourcePos;
};

class QMT_EXPORT TextScanner : public QObject
{
    class TextScannerPrivate;

public:
    explicit TextScanner(QObject *parent = nullptr);
    ~TextScanner() override;

    void setKeywords(const QList<QPair<QString, int>> &keywords);
    void setOperators(const QList<QPair<QString, int>> &operators);
    void setSource(ITextSource *textSource);
    SourcePos sourcePos() const;

    Token read();
    void unread(const Token &token);

private:
    SourceChar readChar();
    void unreadChar(const SourceChar &sourceChar);
    void skipWhitespaces();

    Token scanString(const SourceChar &delimiterChar);
    Token scanNumber(const SourceChar &firstDigit);
    Token scanIdentifier(const SourceChar &firstChar);
    Token scanColorIdentifier(const SourceChar &firstChar);
    Token scanOperator(const SourceChar &firstChar);

    TextScannerPrivate *d;
};

} // namespace qmt
