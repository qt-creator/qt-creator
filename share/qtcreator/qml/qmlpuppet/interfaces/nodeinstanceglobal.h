// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QByteArray>
#include <QList>

#include <vector>

namespace QmlDesigner {

using PropertyName = QByteArray;
using PropertyNameList = QList<PropertyName>;
using PropertyNames = std::vector<PropertyName>;
using TypeName = QByteArray;

enum class AuxiliaryDataType {
    None,
    Temporary,
    Document,
    NodeInstancePropertyOverwrite,
    NodeInstanceAuxiliary
};
}
