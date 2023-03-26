// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "resultparser.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <utility>

namespace GitLab {


QString Event::toMessage() const
{
    QString message;
    if (author.realname.isEmpty())
        message.append(author.name);
    else
        message.append(author.realname + " (" + author.name + ')');
    message.append(' ');
    if (!pushData.isEmpty())
        message.append(pushData);
    else if (!targetTitle.isEmpty())
        message.append(action + ' ' + targetType + " '" + targetTitle +'\'');
    else
        message.append(action + ' ' + targetType);
    return message;
}

namespace ResultParser {

static PageInformation paginationInformation(const QByteArray &header)
{
    PageInformation result;
    const QByteArrayList lines = header.split('\n');
    for (const QByteArray &line : lines) {
        const QByteArray lower = line.toLower(); // depending on OS this may be capitalized
        if (lower.startsWith("x-page: "))
            result.currentPage = line.mid(8).toInt();
        else if (lower.startsWith("x-per-page: "))
          result.perPage = line.mid(12).toInt();
        else if (lower.startsWith("x-total: "))
          result.total = line.mid(9).toInt();
        else if (lower.startsWith("x-total-pages: "))
          result.totalPages = line.mid(15).toInt();
    }
    return result;
}

static std::pair<QByteArray, QByteArray> splitHeaderAndBody(const QByteArray &input)
{
    QByteArray header;
    QByteArray json;
    int emptyLine = input.indexOf("\r\n\r\n"); // we always get \r\n as line separator?
    if (emptyLine != -1) {
        header = input.left(emptyLine);
        json = input.mid(emptyLine + 4);
    } else {
        json = input;
    }
    return {header, json};
}

static std::pair<Error, QJsonObject> preHandleSingle(const QByteArray &json)
{
    Error result;
    QJsonObject object;
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &error);

    if (error.error != QJsonParseError::NoError) {
        if (!json.isEmpty() && json.at(0) == '<') // we likely got an HTML response
            result.code = 399;
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

    return {result, object};
}

static std::pair<Error, QJsonDocument> preHandleHeaderAndBody(const QByteArray &header,
                                                              const QByteArray &json)
{
    Error result;
    if (header.isEmpty()) {
        result.message = "Missing Expected Header";
        return {result, {}};
    }

    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &error);
    if (error.error != QJsonParseError::NoError) {
        result.message = error.errorString();
        return {result, doc};
    }

    if (doc.isObject()) {
        const QJsonObject obj = doc.object();
        if (obj.contains("message")) {
            result = parseErrorMessage(obj.value("message").toString());
            return {result, doc};
        } else if (obj.contains("error")) {
            if (obj.value("error").toString() == "insufficient_scope")
                result.code = 1;
            result.message = obj.value("error_description").toString();
            return {result, doc};
        }
    }

    if (!doc.isArray())
        result.message = "Not an Array";

    return {result, doc};
}

static User userFromJson(const QJsonObject &jsonObj)
{
    User user;
    user.name = jsonObj.value("username").toString();
    user.realname = jsonObj.value("name").toString();
    user.id = jsonObj.value("id").toInt(-1);
    user.email = jsonObj.value("email").toString();
    user.lastLogin = jsonObj.value("last_sign_in_at").toString();
    user.bot = jsonObj.value("bot").toBool();
    return user;
}

static Project projectFromJson(const QJsonObject &jsonObj)
{
    Project project;
    project.name = jsonObj.value("name").toString();
    project.displayName = jsonObj.value("name_with_namespace").toString();
    project.pathName = jsonObj.value("path_with_namespace").toString();
    project.id = jsonObj.value("id").toInt(-1);
    project.visibility = jsonObj.value("visibility").toString();
    project.httpUrl = jsonObj.value("http_url_to_repo").toString();
    project.sshUrl = jsonObj.value("ssh_url_to_repo").toString();
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

static Event eventFromJson(const QJsonObject &jsonObj)
{
    Event event;
    event.action = jsonObj.value("action_name").toString();
    const QJsonValue value = jsonObj.value("target_type");
    event.targetType = value.isNull() ? "project" : jsonObj.value("target_type").toString();
    if (event.targetType == "DiffNote") {
        const QJsonObject noteObject = jsonObj.value("note").toObject();
        event.targetType = noteObject.value("noteable_type").toString();
    }
    event.targetTitle = jsonObj.value("target_title").toString();
    event.author = userFromJson(jsonObj.value("author").toObject());
    event.timeStamp = jsonObj.value("created_at").toString();
    if (jsonObj.contains("push_data")) {
        const QJsonObject pushDataObj = jsonObj.value("push_data").toObject();
        if (!pushDataObj.isEmpty()) {
            const QString action = pushDataObj.value("action").toString();
            const QString ref = pushDataObj.value("ref").toString();
            const QString refType = pushDataObj.value("ref_type").toString();
            event.pushData = action + ' ' + refType + " '" + ref + '\'';
        }
     }
    return event;
}

User parseUser(const QByteArray &input)
{
    auto [error, userObj] = preHandleSingle(input);
    if (!error.message.isEmpty()) {
        User result;
        result.error = error;
        return result;
    }
    return userFromJson(userObj);
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

Projects parseProjects(const QByteArray &input)
{
    auto [header, json] = splitHeaderAndBody(input);
    auto [error, jsonDoc] = preHandleHeaderAndBody(header, json);
    Projects result;
    if (!error.message.isEmpty()) {
        result.error = error;
        return result;
    }
    result.pageInfo = paginationInformation(header);
    const QJsonArray projectsArray = jsonDoc.array();
    for (const QJsonValue &value : projectsArray) {
        if (!value.isObject())
            continue;
        const QJsonObject projectObj = value.toObject();
        result.projects.append(projectFromJson(projectObj));
    }
    return result;
}

Events parseEvents(const QByteArray &input)
{
    auto [header, json] = splitHeaderAndBody(input);
    auto [error, jsonDoc] = preHandleHeaderAndBody(header, json);
    Events result;
    if (!error.message.isEmpty()) {
        result.error = error;
        return result;
    }
    result.pageInfo = paginationInformation(header);
    const QJsonArray eventsArray = jsonDoc.array();
    for (const QJsonValue &value : eventsArray) {
        if (!value.isObject())
            continue;
        const QJsonObject eventObj = value.toObject();
        result.events.append(eventFromJson(eventObj));
    }
    return result;

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
