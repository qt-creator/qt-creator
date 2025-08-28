// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "dashboard/dto.h"

#include <utils/id.h>
#include <utils/result.h>

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

namespace Utils {
class Environment;
class FilePath;
}

namespace Axivion::Internal {

constexpr int DefaultSearchLimit = 2048;

enum class DashboardMode { Global, Local };

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
    std::optional<QUrl> globalNamedFilters;
    std::optional<QUrl> userNamedFilters;
    std::optional<QString> userName;
};

enum class ContentType {
    Html,
    Json,
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

QUrl resolveDashboardInfoUrl(DashboardMode dashboardMode, const QUrl &url);

Tasking::Group downloadDataRecipe(DashboardMode dashboardMode,
                                  const Tasking::Storage<DownloadData> &storage);

using DashboardInfoHandler = std::function<void(const Utils::Result<DashboardInfo> &)>;
Tasking::Group dashboardInfoRecipe(DashboardMode dashboardMode,
                                   const DashboardInfoHandler &handler = {});

Tasking::Group projectInfoRecipe(DashboardMode dashboardMode, const QString &projectName);

// TODO: Wrap into Result<>?
using TableInfoHandler = std::function<void(const Dto::TableInfoDto &)>;
Tasking::Group tableInfoRecipe(DashboardMode dashboardMode,
                               const QString &prefix, const TableInfoHandler &handler);

// TODO: Wrap into Result<>?
using IssueTableHandler = std::function<void(const Dto::IssueTableDto &)>;
Tasking::Group issueTableRecipe(DashboardMode dashboardMode,
                                const IssueListSearch &search, const IssueTableHandler &handler);

// TODO: Wrap into Result<>?
using LineMarkerHandler = std::function<void(const Dto::FileViewDto &)>;
Tasking::Group lineMarkerRecipe(DashboardMode dashboardMode,
                                const Utils::FilePath &filePath, const LineMarkerHandler &handler);

void fetchLocalDashboardInfo(const DashboardInfoHandler &handler, const QString &projectName);
void fetchDashboardAndProjectInfo(const DashboardInfoHandler &handler, const QString &projectName);
std::optional<Dto::ProjectInfoDto> projectInfo();
std::optional<Dto::ProjectInfoDto> localProjectInfo();

struct NamedFilter
{
    QString key;
    QString displayName;
    bool global = false;
};

void fetchNamedFilters(DashboardMode dashboardMode);
QList<NamedFilter> knownNamedFiltersFor(const QString &issueKind, bool global);
std::optional<Dto::NamedFilterInfoDto> namedFilterInfoForKey(const QString &key, bool global);

bool handleCertificateIssue();

QIcon iconForIssue(const std::optional<Dto::IssueKind> &issueKind);
QString anyToSimpleString(const Dto::Any &any, const QString &type,
                          const std::optional<std::vector<Dto::ColumnTypeOptionDto>> &options);
void fetchIssueInfo(DashboardMode dashboardMode, const QString &id);

void switchActiveDashboardId(const Utils::Id &toDashboardId);
const Utils::Id activeDashboardId();
const std::optional<DashboardInfo> currentDashboardInfo();
void setAnalysisVersion(const QString &version);
void enableInlineIssues(bool enable);

void switchDashboardMode(DashboardMode mode, bool byLocalBuildButton); // FIXME
DashboardMode currentDashboardMode();

Utils::FilePath findFileForIssuePath(const Utils::FilePath &issuePath);

void updateEnvironmentForLocalBuild(Utils::Environment *env);

} // Axivion::Internal

Q_DECLARE_METATYPE(Axivion::Internal::NamedFilter)
