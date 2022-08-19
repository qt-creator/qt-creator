// Copyright (C) 2016 Denis Mingulov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QMetaType>
#include <QString>

namespace ClassView {
namespace Internal {

class SymbolLocation
{
public:
    //! Default constructor
    SymbolLocation();

    //! Constructor
    explicit SymbolLocation(const QString &file, int lineNumber = 0, int columnNumber = 0);

    inline const QString &fileName() const { return m_fileName; }
    inline int line() const { return m_line; }
    inline int column() const { return m_column; }
    inline size_t hash() const { return m_hash; }
    inline bool operator==(const SymbolLocation &other) const
    {
        return hash() == other.hash() && line() == other.line() && column() == other.column()
            && fileName() == other.fileName();
    }

private:
    const QString m_fileName;
    const int m_line;
    const int m_column;
    const size_t m_hash; // precalculated hash value - to speed up qHash
};

//! qHash overload for QHash/QSet
inline size_t qHash(const ClassView::Internal::SymbolLocation &location)
{
    return location.hash();
}

} // namespace Internal
} // namespace ClassView

Q_DECLARE_METATYPE(ClassView::Internal::SymbolLocation)
