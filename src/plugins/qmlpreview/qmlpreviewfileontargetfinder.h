// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QUrl>
#include <QString>
#include <QPointer>

namespace ProjectExplorer { class BuildConfiguration; }

namespace QmlPreview {

class QmlPreviewFileOnTargetFinder
{
public:
    void setBuildConfiguration(ProjectExplorer::BuildConfiguration *bc);

    QString findPath(const QString &filePath, bool *success = nullptr) const;
    QUrl findUrl(const QString &filePath, bool *success = nullptr) const;

private:
    QPointer<ProjectExplorer::BuildConfiguration> m_buildConfig;
};

} // namespace QmlPreview
