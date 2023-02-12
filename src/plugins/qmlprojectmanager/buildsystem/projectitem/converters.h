// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QJsonObject>
#include <QString>

#include <utils/environment.h>
#include <qmljs/qmljssimplereader.h>

#include <QJsonArray>

namespace QmlProjectManager::Converters {

QString jsonToQmlProject(const QJsonObject &rootObject);
QJsonObject qmlProjectTojson(const Utils::FilePath &projectFile);

} // namespace QmlProjectManager::Converters
