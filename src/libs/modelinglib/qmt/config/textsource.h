// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sourcepos.h"

#include <QChar>

namespace qmt {

class SourceChar
{
public:
    SourceChar() = default;
    SourceChar(QChar chr, const SourcePos &sourcePos)
        : ch(chr),
          pos(sourcePos)
    {
    }

    QChar ch;
    SourcePos pos;
};

class ITextSource
{
public:
    virtual ~ITextSource() {}

    virtual SourceChar readNextChar() = 0;
};

} // namespace qmt
