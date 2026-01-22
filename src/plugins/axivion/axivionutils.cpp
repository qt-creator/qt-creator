// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "axivionutils.h"

#include "axivionsettings.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/idocument.h>

#include <utils/algorithm.h>
#include <utils/filepath.h>

#include <QCoreApplication>
#include <QNetworkReply>

using namespace Utils;

namespace Axivion::Internal {

QByteArray contentTypeData(ContentType contentType)
{
    switch (contentType) {
    case ContentType::Html:      return s_htmlContentType;
    case ContentType::Json:      return s_jsonContentType;
    case ContentType::PlainText: return s_plaintextContentType;
    case ContentType::Svg:       return s_svgContentType;
    }
    return {};
}

QByteArray contentTypeFromRawHeader(QNetworkReply *reply)
{
    return reply->header(QNetworkRequest::ContentTypeHeader).toByteArray()
            .split(';').constFirst().trimmed().toLower();
}

QByteArray fixIssueDetailsHtml(const QByteArray &original)
{
    QByteArray fixedHtml = original;
    const int idx = fixedHtml.indexOf("<div class=\"ax-issuedetails-table-container\">");
    if (idx >= 0)
        fixedHtml = "<html><body>" + fixedHtml.mid(idx);
    return fixedHtml;
}

QByteArray axivionUserAgent()
{
    return QByteArray{"Axivion" + QCoreApplication::applicationName().toUtf8()
                + "Plugin/" + QCoreApplication::applicationVersion().toUtf8()};
}

bool saveModifiedFiles(const QString &projectName)
{
    QList<Core::IDocument *> modifiedDocs = Core::DocumentManager::modifiedDocuments();
    if (modifiedDocs.isEmpty())
        return true;

    // if we have a mapping, limit to docs of this project directory, otherwise save all
    const FilePath projectBase = settings().localProjectForProjectName(projectName);
    if (!projectBase.isEmpty()) {
        modifiedDocs = Utils::filtered(modifiedDocs, [projectBase](Core::IDocument *doc) {
                return doc->filePath().isChildOf(projectBase);
        });
    }
    bool canceled = false;
    bool success = Core::DocumentManager::saveModifiedDocumentsSilently(modifiedDocs, &canceled);
    return success && !canceled;
}


} // namespace Axivion::Internal
