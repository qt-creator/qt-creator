// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "dashboard/dto.h"

#include <utils/expected.h>

#include <QHash>
#include <QUrl>
#include <QVersionNumber>

#include <memory>

QT_BEGIN_NAMESPACE
class QIcon;
QT_END_NAMESPACE

namespace ProjectExplorer { class Project; }

namespace Tasking { class Group; }

namespace Axivion::Internal {

struct IssueListSearch
{
    QString kind;
    QString state;
    QString versionStart;
    QString versionEnd;
    QString owner;
    QString pathglob;
    int offset = 0;
    int limit = 30;
    bool computeTotalRowCount = false;

    QString toQuery() const;
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

using DashboardInfoHandler = std::function<void(const Utils::expected_str<DashboardInfo> &)>;
Tasking::Group dashboardInfoRecipe(const DashboardInfoHandler &handler = {});

// TODO: Wrap into expected_str<>?
using TableInfoHandler = std::function<void(const Dto::TableInfoDto &)>;
Tasking::Group tableInfoRecipe(const QString &prefix, const TableInfoHandler &handler);

// TODO: Wrap into expected_str<>?
using IssueTableHandler = std::function<void(const Dto::IssueTableDto &)>;
Tasking::Group issueTableRecipe(const IssueListSearch &search, const IssueTableHandler &handler);

void fetchProjectInfo(const QString &projectName);
std::optional<Dto::ProjectInfoDto> projectInfo();
bool handleCertificateIssue();

QIcon iconForIssue(const QString &prefix);

} // Axivion::Internal

