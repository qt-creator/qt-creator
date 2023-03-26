// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QDateTime>
#include <QHash>
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
};
using ClazyChecks = QVector<ClazyCheck>;

class ClazyStandaloneInfo
{
public:
    static ClazyStandaloneInfo getInfo(const Utils::FilePath &executablePath);

    QVersionNumber version;
    QStringList defaultChecks;
    ClazyChecks supportedChecks;

private:
    ClazyStandaloneInfo(const Utils::FilePath &executablePath);

    static QHash<Utils::FilePath, QPair<QDateTime, ClazyStandaloneInfo>> cache;
};

} // namespace Internal
} // namespace ClangTools

