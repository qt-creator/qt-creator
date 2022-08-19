// Copyright (C) 2016 Denis Mingulov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "classviewsymbollocation.h"
#include "classviewsymbolinformation.h"

#include <QVariant>
#include <QList>
#include <QSet>

QT_FORWARD_DECLARE_CLASS(QStandardItem)

namespace ClassView {
namespace Internal {

QSet<SymbolLocation> roleToLocations(const QList<QVariant> &locations);
SymbolInformation symbolInformationFromItem(const QStandardItem *item);

} // namespace Internal
} // namespace ClassView
