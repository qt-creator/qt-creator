/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include <clangsupport/filepath.h>

#include <utils/linecolumn.h>

namespace ClangRefactoring {

using SymbolString = Utils::BasicSmallString<63>;
using SignatureString = Utils::BasicSmallString<126>;
using SymbolId = long long;

class Symbol
{
public:
    Symbol() = default;
    Symbol(SymbolId symbolId, Utils::SmallStringView name)
        : name(name), symbolId(symbolId)
    {}
    Symbol(SymbolId symbolId,  Utils::SmallStringView name,  Utils::SmallStringView signature)
        : signature(signature), name(name), symbolId(symbolId)
    {}
    SignatureString signature;
    SymbolString name;
    SymbolId symbolId;

    friend
    bool operator==(const Symbol &first, const Symbol &second)
    {
        return first.symbolId == second.symbolId
            && first.name == second.name
            && first.signature == second.signature;
    }
};

using Symbols = std::vector<Symbol>;

} // namespace ClangRefactoring

Q_DECLARE_METATYPE(ClangRefactoring::Symbol)
