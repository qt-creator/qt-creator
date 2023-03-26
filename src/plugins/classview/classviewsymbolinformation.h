// Copyright (C) 2016 Denis Mingulov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMetaType>
#include <QString>

#include <limits.h>

namespace ClassView {
namespace Internal {

class SymbolInformation
{
public:
    SymbolInformation();
    SymbolInformation(const QString &name, const QString &type, int iconType = INT_MIN);

    bool operator<(const SymbolInformation &other) const;

    inline const QString &name() const { return m_name; }
    inline const QString &type() const { return m_type; }
    inline int iconType() const { return m_iconType; }
    inline auto hash() const { return m_hash; }
    inline bool operator==(const SymbolInformation &other) const
    {
        return hash() == other.hash() && iconType() == other.iconType() && name() == other.name()
            && type() == other.type();
    }

    int iconTypeSortOrder() const;

    friend auto qHash(const SymbolInformation &information) { return information.hash(); }

private:
    const int m_iconType;
    const size_t m_hash; // precalculated hash value - to speed up qHash
    const QString m_name;               // symbol name (e.g. SymbolInformation)
    const QString m_type;               // symbol type (e.g. (int char))
};

} // namespace Internal
} // namespace ClassView

Q_DECLARE_METATYPE(ClassView::Internal::SymbolInformation)
