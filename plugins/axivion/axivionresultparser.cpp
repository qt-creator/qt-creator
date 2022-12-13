// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include "axivionresultparser.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace Axivion::Internal {


namespace ResultParser {

DashboardInfo parseDashboardInfo(const QByteArray &input)
{
    DashboardInfo result;

    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(input, &error);
    if (error.error != QJsonParseError::NoError) {
        if (input.startsWith("Authentication failed:"))
            result.error = QString::fromUtf8(input);
        else
            result.error = error.errorString();
        return result;
    }

    if (!doc.isObject()) {
        result.error = "Not an object.";
        return result;
    }
    const QJsonObject object = doc.object();
    result.mainUrl = object.value("mainUrl").toString();

    if (!object.contains("projects")) {
        result.error = "Missing projects information.";
        return result;
    }
    const QJsonValue projects = object.value("projects");
    if (!projects.isArray()) {
        result.error = "Projects information not an array.";
        return result;
    }
    const QJsonArray array = projects.toArray();
    for (const QJsonValue &val : array) {
        if (!val.isObject())
            continue;
        const QJsonObject projectObject = val.toObject();
        Project project;
        project.name = projectObject.value("name").toString();
        project.url = projectObject.value("url").toString();
        if (project.name.isEmpty() || project.url.isEmpty())
            continue;
        result.projects.append(project);
    }
    return result;
}

} // ResultParser

} // Axivion::Internal
