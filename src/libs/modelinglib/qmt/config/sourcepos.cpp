// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sourcepos.h"

namespace qmt {

SourcePos::SourcePos()
{
}

SourcePos::SourcePos(int sourceId, int lineNumber, int columnNumber)
    : m_sourceId(sourceId),
      m_lineNumber(lineNumber),
      m_columnNumber(columnNumber)
{
}

} // namespace qmt
