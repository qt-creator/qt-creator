// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "axivionresultparser.h"

#include <utils/qtcassert.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

#include <stdexcept>
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
