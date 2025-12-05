// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QPair>
#include <QStringList>
#include <QVector>
#include <QVersionNumber>

namespace ClangTools {
namespace Internal {

QPair<Utils::FilePath, QString> getClangIncludeDirAndVersion(const Utils::FilePath &clangToolPath);

enum class QueryFailMode{ Silent, Noisy };
QString queryVersion(const Utils::FilePath &clangToolPath, QueryFailMode failMode);

class ClangTidyInfo
{
public:
    ClangTidyInfo(const Utils::FilePath &executablePath);
    QStringList defaultChecks;
    QStringList supportedChecks;
};

class ClazyCheck
{
public:
    QString name;
    int level;
    QStringList topics;

    friend bool operator==(const ClazyCheck &c1, const ClazyCheck &c2);
    friend bool operator!=(const ClazyCheck &c1, const ClazyCheck &c2);
};
using ClazyChecks = QVector<ClazyCheck>;

class ClazyStandaloneInfo
{
public:
    ClazyStandaloneInfo(const Utils::FilePath &executablePath);

    QVersionNumber version;
    QStringList defaultChecks;
    ClazyChecks supportedChecks;

    friend bool operator==(const ClazyStandaloneInfo &c1, const ClazyStandaloneInfo &c2);
    friend bool operator!=(const ClazyStandaloneInfo &c1, const ClazyStandaloneInfo &c2);
};

} // namespace Internal
} // namespace ClangTools

