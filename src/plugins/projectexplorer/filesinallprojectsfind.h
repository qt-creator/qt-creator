// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "allprojectsfind.h"

namespace ProjectExplorer {
namespace Internal {

class FilesInAllProjectsFind : public AllProjectsFind
{
    Q_OBJECT

public:
    QString id() const override;
    QString displayName() const override;

    void writeSettings(QSettings *settings) override;
    void readSettings(QSettings *settings) override;

protected:
    Utils::FileIterator *files(const QStringList &nameFilters,
                               const QStringList &exclusionFilters,
                               const QVariant &additionalParameters) const override;
    QString label() const override;
};

} // namespace Internal
} // namespace ProjectExplorer

