// Copyright (C) 2016 Denis Mingulov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QMetaType>

namespace ClassView::Internal {

class SymbolLocation
{
public:
    //! Default constructor
    SymbolLocation();

    //! Constructor
    explicit SymbolLocation(const Utils::FilePath &file, int lineNumber = 0, int columnNumber = 0);

    const Utils::FilePath &filePath() const { return m_fileName; }
    int line() const { return m_line; }
    int column() const { return m_column; }
    size_t hash() const { return m_hash; }

    bool operator==(const SymbolLocation &other) const
    {
        return hash() == other.hash() && line() == other.line() && column() == other.column()
            && filePath() == other.filePath();
    }

private:
    const Utils::FilePath m_fileName;
    const int m_line;
    const int m_column;
    const size_t m_hash; // precalculated hash value - to speed up qHash
};

//! qHash overload for QHash/QSet
inline size_t qHash(const ClassView::Internal::SymbolLocation &location)
{
    return location.hash();
}

} // ClassView::Internal

Q_DECLARE_METATYPE(ClassView::Internal::SymbolLocation)
