// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QByteArray>

QT_BEGIN_NAMESPACE
class QNetworkReply;
QT_END_NAMESPACE

namespace Axivion::Internal {

constexpr char s_htmlContentType[]      = "text/html";
constexpr char s_plaintextContentType[] = "text/plain";
constexpr char s_svgContentType[]       = "image/svg+xml";
constexpr char s_jsonContentType[]      = "application/json";

enum class ContentType {
    Html,
    Json,
    PlainText,
    Svg
};

QByteArray contentTypeData(ContentType contentType);
QByteArray contentTypeFromRawHeader(QNetworkReply *reply);
QByteArray fixIssueDetailsHtml(const QByteArray &original);

QByteArray axivionUserAgent();

bool saveModifiedFiles(const QString &projectName);

} // namespace Axivion::Internal
