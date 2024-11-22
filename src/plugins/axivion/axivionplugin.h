// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "dashboard/dto.h"

#include <utils/expected.h>
#include <utils/id.h>

#include <QHash>
#include <QMap>
#include <QUrl>
#include <QVersionNumber>

QT_BEGIN_NAMESPACE
class QIcon;
QT_END_NAMESPACE

namespace ProjectExplorer { class Project; }

namespace Tasking {
class Group;
template <typename StorageStruct>
class Storage;
}

namespace Utils { class FilePath; }

namespace Axivion::Internal {

constexpr int DefaultSearchLimit = 2048;

enum class QueryMode {
    SimpleQuery,            // just kind and version start and end
    FilterQuery,            // + all filters if available
    FullQuery               // + offset, limit, computeTotalRowCount
};

struct IssueListSearch
{
    QString kind;
    QString state;
    QString versionStart;
    QString versionEnd;
    QString owner;
    QString filter_path;
    QString sort;
    QMap<QString, QString> filter;
    int offset = 0;
    int limit = DefaultSearchLimit;
    bool computeTotalRowCount = false;

    QUrlQuery toUrlQuery(QueryMode mode) const;
};

class DashboardInfo
{
public:
    QUrl source;
    QVersionNumber versionNumber;
    QStringList projects;
    QHash<QString, QUrl> projectUrls;
    std::optional<QUrl> checkCredentialsUrl;
};

enum class ContentType {
    Html,
    PlainText,
    Svg
};

class DownloadData
{
public:
    QUrl inputUrl;
    ContentType expectedContentType = ContentType::Html;
    QByteArray outputData;
};

Tasking::Group downloadDataRecipe(const Tasking::Storage<DownloadData> &storage);

using DashboardInfoHandler = std::function<void(const Utils::expected_str<DashboardInfo> &)>;
Tasking::Group dashboardInfoRecipe(const DashboardInfoHandler &handler = {});

Tasking::Group projectInfoRecipe(const QString &projectName);

// TODO: Wrap into expected_str<>?
using TableInfoHandler = std::function<void(const Dto::TableInfoDto &)>;
Tasking::Group tableInfoRecipe(const QString &prefix, const TableInfoHandler &handler);

// TODO: Wrap into expected_str<>?
using IssueTableHandler = std::function<void(const Dto::IssueTableDto &)>;
Tasking::Group issueTableRecipe(const IssueListSearch &search, const IssueTableHandler &handler);

// TODO: Wrap into expected_str<>?
using LineMarkerHandler = std::function<void(const Dto::FileViewDto &)>;
Tasking::Group lineMarkerRecipe(const Utils::FilePath &filePath, const LineMarkerHandler &handler);

using HtmlHandler = std::function<void(const QByteArray &)>;
Tasking::Group issueHtmlRecipe(const QString &issueId, const HtmlHandler &handler);

void fetchDashboardAndProjectInfo(const DashboardInfoHandler &handler, const QString &projectName);
std::optional<Dto::ProjectInfoDto> projectInfo();
bool handleCertificateIssue();

QIcon iconForIssue(const std::optional<Dto::IssueKind> &issueKind);
QString anyToSimpleString(const Dto::Any &any);
void fetchIssueInfo(const QString &id);

void switchActiveDashboardId(const Utils::Id &toDashboardId);
const Utils::Id activeDashboardId();
const std::optional<DashboardInfo> currentDashboardInfo();
void setAnalysisVersion(const QString &version);
void enableInlineIssues(bool enable);

Utils::FilePath findFileForIssuePath(const Utils::FilePath &issuePath);

} // Axivion::Internal

