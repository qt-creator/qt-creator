/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "sourcelocations.h"

#include "class.h"
#include "enum.h"
#include "function.h"
#include "include.h"
#include "symbol.h"

#include <cpptools/usages.h>

namespace ClangRefactoring {

enum class SymbolType
{
    Class = 0,
    Enum = 1,
    Include = 2
};

class SymbolQueryInterface
{
public:
    virtual ~SymbolQueryInterface() {}
    virtual SourceLocations locationsAt(ClangBackEnd::FilePathId filePathId,
                                        int line,
                                        int utf8Column) const = 0;
    virtual CppTools::Usages sourceUsagesAt(ClangBackEnd::FilePathId filePathId,
                                            int line,
                                            int utf8Column) const = 0;
    virtual Symbols symbolsContaining(SymbolType symbolType,
                                      Utils::SmallStringView regularExpression) const = 0;
    virtual Functions functionsContaining(Utils::SmallStringView regularExpression) const = 0;
};

} // namespace ClangRefactoring
