// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "textsource.h"

#include <QString>

namespace qmt {

class QMT_EXPORT StringTextSource : public ITextSource
{
public:
    StringTextSource();
    ~StringTextSource();

    void setText(const QString &text);
    int sourceId() const { return m_sourceId; }
    void setSourceId(int sourceId);

    // ITextSource interface
    SourceChar readNextChar() override;

private:
    QString m_text;
    int m_sourceId = -1;
    int m_index = -1;
    int m_lineNumber = -1;
    int m_columnNumber = -1;
};

} // namespace qmt
