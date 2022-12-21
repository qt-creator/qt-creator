// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clientrequests.h"

#include <QDebug>
#include <QMetaEnum>

namespace Nim {
namespace Suggest {

bool Line::fromString(Line::LineType &type, const std::string &str)
{
    static const auto metaobject = QMetaEnum::fromType<LineType>();
    bool result = false;
    type = static_cast<LineType>(metaobject.keyToValue(str.c_str(), &result));
    return result;
}

bool Line::fromString(Line::SymbolKind &type, const std::string &str)
{
    static const auto metaobject = QMetaEnum::fromType<SymbolKind>();
    bool result = false;
    type = static_cast<SymbolKind>(metaobject.keyToValue(str.c_str(), &result));
    return result;
}

NimSuggestClientRequest::NimSuggestClientRequest(quint64 id)
    : m_id(id)
{}

QDebug operator<<(QDebug debug, const Line &c)
{
    QDebugStateSaver saver(debug);
    debug.space() << c.line_type << c.symbol_kind << c.symbol_type << c.data << c.row << c.column <<
                  c.abs_path;
    return debug;
}

} // namespace Suggest
} // namespace Nim
