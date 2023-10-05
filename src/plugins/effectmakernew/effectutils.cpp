// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "effectutils.h"

#include <QJsonArray>

namespace EffectMaker {

QString EffectUtils::codeFromJsonArray(const QJsonArray &codeArray)
{
    if (codeArray.isEmpty())
        return {};

    QString codeString;
    for (const auto &element : codeArray)
        codeString += element.toString() + '\n';

    codeString.chop(1); // Remove last '\n'
    return codeString;
}

} // namespace EffectMaker

