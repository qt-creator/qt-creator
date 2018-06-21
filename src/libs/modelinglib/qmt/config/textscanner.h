/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <QObject>

#include "sourcepos.h"

#include "qmt/infrastructure/exceptions.h"

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
    Q_OBJECT
    class TextScannerPrivate;

public:
    explicit TextScanner(QObject *parent = nullptr);
    ~TextScanner() override;

    void setKeywords(const QList<QPair<QString, int> > &keywords);
    void setOperators(const QList<QPair<QString, int> > &operators);
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
