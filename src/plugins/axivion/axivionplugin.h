// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "dashboard/dto.h"

#include <memory>

QT_BEGIN_NAMESPACE
class QIcon;
QT_END_NAMESPACE

namespace ProjectExplorer { class Project; }

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

    QString toQuery() const;
};

void fetchProjectInfo(const QString &projectName);
std::optional<Dto::ProjectInfoDto> projectInfo();
void fetchIssueTableLayout(const QString &prefix);
void fetchIssues(const IssueListSearch &search);
bool handleCertificateIssue();

QIcon iconForIssue(const QString &prefix);

} // Axivion::Internal

