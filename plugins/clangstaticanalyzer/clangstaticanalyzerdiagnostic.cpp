/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com <http://qt.digia.com/>
**
** This file is part of the Qt Enterprise LicenseChecker Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com <http://qt.digia.com/>
**
****************************************************************************/

#include "clangstaticanalyzerdiagnostic.h"

namespace ClangStaticAnalyzer {
namespace Internal {

Location::Location()
    : line(0), column(0)
{
}

Location::Location(const QString &filePath, int line, int column)
    : filePath(filePath), line(line), column(column)
{
}

bool Location::isValid() const
{
    return !filePath.isEmpty() && line >= 0 && column >= 0;
}

ExplainingStep::ExplainingStep()
    : depth(0)
{
}

bool ExplainingStep::isValid() const
{
    return location.isValid() && !ranges.isEmpty() && !message.isEmpty();
}

} // namespace Internal
} // namespace ClangStaticAnalyzer
