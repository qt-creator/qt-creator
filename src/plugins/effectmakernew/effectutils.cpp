// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "effectutils.h"

#include <coreplugin/icore.h>

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

QString EffectUtils::nodesSourcesPath()
{
#ifdef SHARE_QML_PATH
    if (Utils::qtcEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/effectMakerNodes";
#endif
    return Core::ICore::resourcePath("qmldesigner/effectMakerNodes").toString();
}

} // namespace EffectMaker

