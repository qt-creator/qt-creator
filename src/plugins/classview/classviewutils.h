// Copyright (C) 2016 Denis Mingulov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "classviewsymbolinformation.h"
#include "classviewsymbollocation.h"

#include <QList>
#include <QSet>
#include <QVariant>

QT_BEGIN_NAMESPACE
class QStandardItem;
QT_END_NAMESPACE

namespace ClassView {
namespace Internal {

QSet<SymbolLocation> roleToLocations(const QList<QVariant> &locations);
SymbolInformation symbolInformationFromItem(const QStandardItem *item);

} // namespace Internal
} // namespace ClassView
