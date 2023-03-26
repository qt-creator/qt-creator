// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmt/infrastructure/qmt_global.h"

namespace qmt {

class QMT_EXPORT SourcePos
{
public:
    SourcePos();
    SourcePos(int sourceId, int lineNumber, int columnNumber = -1);

    bool isValid() const { return m_sourceId >= 0 && m_lineNumber >= 0; }
    int sourceId() const { return m_sourceId; }
    int lineNumber() const { return m_lineNumber; }
    bool hasColumnNumber() const { return m_columnNumber >= 0; }
    int columnNumber() const { return m_columnNumber; }

private:
    int m_sourceId = -1;
    int m_lineNumber = -1;
    int m_columnNumber = -1;
};

} // namespace qmt
