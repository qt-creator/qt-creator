// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "axivionresultparser.h"

#include <utils/qtcassert.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

#include <utility>

namespace Axivion::Internal {

static std::pair<QByteArray, QByteArray> splitHeaderAndBody(const QByteArray &input)
{
    QByteArray header;
    QByteArray json;
    int emptyLine = input.indexOf("\r\n\r\n"); // we always get \r\n as line separator
    if (emptyLine != -1) {
        header = input.left(emptyLine);
        json = input.mid(emptyLine + 4);
    } else {
        json = input;
    }
    return {header, json};
}

static int httpStatus(const QByteArray &header)
{
    int firstHeaderEnd = header.indexOf("\r\n");
    if (firstHeaderEnd == -1)
        return 600; // unexpected header
    const QString firstLine = QString::fromUtf8(header.first(firstHeaderEnd));
    static const QRegularExpression regex(R"(^HTTP/\d\.\d (\d{3}) .*$)");
    const QRegularExpressionMatch match = regex.match(firstLine);
    return match.hasMatch() ? match.captured(1).toInt() : 601;
}

static BaseResult prehandleHeader(const QByteArray &header, const QByteArray &body)
{
    BaseResult result;
    if (header.isEmpty()) {
        result.error = QString::fromUtf8(body); // we likely had a curl problem
        return result;
    }
    int status = httpStatus(header);
    if ((status > 399) || (status > 299 && body.isEmpty())) { // FIXME handle some explicitly?
        const QString statusStr = QString::number(status);
        if (body.isEmpty() || body.startsWith('<')) // likely an html response or redirect
            result.error = QLatin1String("(%1)").arg(statusStr);
        else
            result.error = QLatin1String("%1 (%2)").arg(QString::fromUtf8(body)).arg(statusStr);
    }
    return result;
}

static std::pair<BaseResult, QJsonDocument> prehandleHeaderAndBody(const QByteArray &header,
                                                                   const QByteArray &body)
{
    BaseResult result = prehandleHeader(header, body);
    if (!result.error.isEmpty())
        return {result, {}};

    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &error);
    if (error.error != QJsonParseError::NoError) {
        result.error = error.errorString();
        return {result, doc};
    }

    if (!doc.isObject()) {
        result.error = "Not an object.";
        return {result, {}};
    }

    return {result, doc};
}

static User::UserType userTypeForString(const QString &type)
{
    if (type == "DASHBOARD_USER")
        return User::Dashboard;
    if (type == "VIRTUAL_USER")
        return User::Virtual;
    return User::Unknown;
}

static User userFromJson(const QJsonObject &object)
{
    User result;
    if (object.isEmpty()) {
        result.error = "Not a user object.";
        return result;
    }
    result.name = object.value("name").toString();
    result.displayName = object.value("displayName").toString();
    result.type = userTypeForString(object.value("type").toString());
    return result;
}

static QList<User> usersFromJson(const QJsonArray &array)
{
    QList<User> result;
    for (const QJsonValue &value : array) {
        User user = userFromJson(value.toObject());
        if (!user.error.isEmpty()) // add this error to result.error?
            continue;
        result.append(user);
    }
    return result;
}

static IssueCount issueCountFromJson(const QJsonObject &object)
{
    IssueCount result;
    if (object.isEmpty()) {
        result.error = "Not an issue count object.";
        return result;
    }
    result.added = object.value("Added").toInt();
    result.removed = object.value("Removed").toInt();
    result.total = object.value("Total").toInt();
    return result;
}

static QList<IssueCount> issueCountsFromJson(const QJsonObject &object)
{
    QList<IssueCount> result;

    const QStringList keys = object.keys();
    for (const QString &k : keys) {
        IssueCount issue = issueCountFromJson(object.value(k).toObject());
        if (!issue.error.isEmpty()) // add this error to result.error?
            continue;
        issue.issueKind = k;
        result.append(issue);
    }
    return result;
}

static ResultVersion versionFromJson(const QJsonObject &object)
{
    ResultVersion result;
    if (object.isEmpty()) {
        result.error = "Not a version object.";
        return result;
    }
    const QJsonValue issuesValue = object.value("issueCounts");
    if (!issuesValue.isObject()) {
        result.error = "Not an object (issueCounts).";
        return result;
    }
    result.issueCounts = issueCountsFromJson(issuesValue.toObject());
    result.timeStamp = object.value("date").toString();
    result.name = object.value("name").toString();
    result.linesOfCode = object.value("linesOfCode").toInt();
    return result;
}

static QList<ResultVersion> versionsFromJson(const QJsonArray &array)
{
    QList<ResultVersion> result;
    for (const QJsonValue &value : array) {
        ResultVersion version = versionFromJson(value.toObject());
        if (!version.error.isEmpty()) // add this error to result.error?
            continue;
        result.append(version);
    }
    return result;
}

static IssueKind issueKindFromJson(const QJsonObject &object)
{
    IssueKind result;
    if (object.isEmpty()) {
        result.error = "Not an issue kind object.";
        return result;
    }
    result.prefix = object.value("prefix").toString();
    result.niceSingular = object.value("niceSingularName").toString();
    result.nicePlural = object.value("nicePluralName").toString();
    return result;
}

static QList<IssueKind> issueKindsFromJson(const QJsonArray &array)
{
    QList<IssueKind> result;
    for (const QJsonValue &value : array) {
        IssueKind kind = issueKindFromJson(value.toObject());
        if (!kind.error.isEmpty()) // add this error to result.error?
            continue;
        result.append(kind);
    }
    return result;
}

namespace ResultParser {

DashboardInfo parseDashboardInfo(const QByteArray &input)
{
    DashboardInfo result;

    auto [header, body] = splitHeaderAndBody(input);
    auto [error, doc] = prehandleHeaderAndBody(header, body);
    if (!error.error.isEmpty()) {
        result.error = error.error;
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

ProjectInfo parseProjectInfo(const QByteArray &input)
{
    ProjectInfo result;

    auto [header, body] = splitHeaderAndBody(input);
    auto [error, doc] = prehandleHeaderAndBody(header, body);
    if (!error.error.isEmpty()) {
        result.error = error.error;
        return result;
    }

    const QJsonObject object = doc.object();
    result.name = object.value("name").toString();

    const QJsonValue usersValue = object.value("users");
    if (!usersValue.isArray()) {
        result.error = "Malformed json response (users).";
        return result;
    }
    result.users = usersFromJson(usersValue.toArray());

    const QJsonValue versionsValue = object.value("versions");
    if (!versionsValue.isArray()) {
        result.error = "Malformed json response (versions).";
        return result;
    }
    result.versions = versionsFromJson(versionsValue.toArray());

    const QJsonValue issueKindsValue = object.value("issueKinds");
    if (!issueKindsValue.isArray()) {
        result.error = "Malformed json response (issueKinds).";
        return result;
    }
    result.issueKinds = issueKindsFromJson(issueKindsValue.toArray());
    return result;
}

static QRegularExpression issueCsvLineRegex(const QByteArray &firstCsvLine)
{
    QString pattern = "^";
    for (const QByteArray &part : firstCsvLine.split(',')) {
        const QString cleaned = QString::fromUtf8(part).remove(' ').chopped(1).mid(1);
        pattern.append(QString("\"(?<" + cleaned + ">.*)\","));
    }
    pattern.chop(1); // remove last comma
    pattern.append('$');
    const QRegularExpression regex(pattern);
    QTC_ASSERT(regex.isValid(), return {});
    return regex;
}

static void parseCsvIssue(const QByteArray &csv, QList<ShortIssue> *issues)
{
    QTC_ASSERT(issues, return);

    bool first = true;
    std::optional<QRegularExpression> regex;
    for (auto &line : csv.split('\n')) {
        if (first) {
            regex.emplace(issueCsvLineRegex(line));
            first = false;
            if (regex.value().pattern().isEmpty())
                return;
            continue;
        }
        if (line.isEmpty())
            continue;
        const QRegularExpressionMatch match = regex->match(QString::fromUtf8(line));
        QTC_ASSERT(match.hasMatch(), continue);
        // FIXME: some of these are not present for all issue kinds! Limited to SV for now
        ShortIssue issue;
        issue.id = match.captured("Id");
        issue.state = match.captured("State");
        issue.errorNumber = match.captured("ErrorNumber");
        issue.message = match.captured("Message");
        issue.entity = match.captured("Entity");
        issue.filePath = match.captured("Path");
        issue.severity = match.captured("Severity");
        issue.lineNumber = match.captured("Line").toInt();
        issues->append(issue);
    }
}

IssuesList parseIssuesList(const QByteArray &input)
{
    IssuesList result;

    auto [header, body] = splitHeaderAndBody(input);
    BaseResult headerResult = prehandleHeader(header, body);
    if (!headerResult.error.isEmpty()) {
        result.error = headerResult.error;
        return result;
    }
    parseCsvIssue(body, &result.issues);
    return result;
}

QString parseRuleInfo(const QByteArray &input) // html result!
{
    auto [header, body] = splitHeaderAndBody(input);
    BaseResult headerResult = prehandleHeader(header, body);
    if (!headerResult.error.isEmpty())
        return QString();
    return QString::fromLocal8Bit(body);
}

} // ResultParser

} // Axivion::Internal
