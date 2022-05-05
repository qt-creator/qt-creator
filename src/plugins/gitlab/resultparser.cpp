/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "resultparser.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <utility>

namespace GitLab {
namespace ResultParser {

static std::pair<Error, QJsonObject> preHandleSingle(const QByteArray &json)
{
    Error result;
    QJsonObject object;
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &error);

    if (error.error != QJsonParseError::NoError) {
        result.message = error.errorString();
    } else if (!doc.isObject()) {
        result.message = "Not an Object";
    } else {
        object = doc.object();
        if (object.contains("message")) {
            result = parseErrorMessage(object.value("message").toString());
        } else if (object.contains("error")) {
            if (object.value("error").toString() == "insufficient_scope")
                result.code = 1;
            result.message = object.value("error_description").toString();
        }
    }

    return std::make_pair(result, object);
}

static Project projectFromJson(const QJsonObject &jsonObj)
{
    Project project;
    project.name = jsonObj.value("name").toString();
    project.displayName = jsonObj.value("name_with_namespace").toString();
    project.pathName = jsonObj.value("path_with_namespace").toString();
    project.id = jsonObj.value("id").toInt(-1);
    project.visibility = jsonObj.value("visibility").toString("public");
    if (jsonObj.contains("forks_count"))
        project.forkCount = jsonObj.value("forks_count").toInt();
    if (jsonObj.contains("star_count"))
        project.starCount = jsonObj.value("star_count").toInt();
    if (jsonObj.contains("open_issues_count"))
        project.issuesCount = jsonObj.value("open_issues_count").toInt();
    const QJsonObject permissions = jsonObj.value("permissions").toObject();
    if (!permissions.isEmpty()) { // separate permissions obj?
        const QJsonObject projAccObj = permissions.value("project_access").toObject();
        if (!projAccObj.isEmpty())
            project.accessLevel = projAccObj.value("access_level").toInt(-1);
    }
    return project;
}

Project parseProject(const QByteArray &input)
{
    auto [error, projectObj] = preHandleSingle(input);
    if (!error.message.isEmpty()) {
        Project result;
        result.error = error;
        return result;
    }
    return projectFromJson(projectObj);
}

Error parseErrorMessage(const QString &message)
{
    Error error;
    bool ok = false;
    error.code = message.left(3).toInt(&ok);
    if (ok)
        error.message = message.mid(4);
    else
        error.message = "Internal Parse Error";
    return error;
}

} // namespace ResultParser
} // namespace GitLab
