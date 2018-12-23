/****************************************************************************
**
** Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
** Contact: http://www.qt.io/licensing
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

BaseNimSuggestClientRequest::BaseNimSuggestClientRequest(quint64 id)
    : m_id(id)
{}

quint64 BaseNimSuggestClientRequest::id() const
{
    return m_id;
}

} // namespace Suggest
} // namespace Nim

QDebug operator<<(QDebug debug, const Nim::Suggest::Line &c)
{
    QDebugStateSaver saver(debug);
    debug.space() << c.line_type << c.symbol_kind << c.symbol_type << c.data << c.row << c.column <<
                  c.abs_path;
    return debug;
}
