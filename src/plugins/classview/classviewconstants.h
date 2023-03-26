// Copyright (C) 2016 Denis Mingulov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <Qt>

namespace ClassView {
namespace Constants {

//! QStandardItem roles
enum ItemRole
{
    SymbolLocationsRole = Qt::UserRole + 1, //!< Symbol locations
    IconTypeRole,                           //!< Icon type (integer)
    SymbolNameRole,                         //!< Symbol name
    SymbolTypeRole                          //!< Symbol type
};

} // namespace Constants
} // namespace ClassView
