// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QFuture>

namespace QmlDesigner::DesignViewer {

class ResourceGeneratorProxy : public QObject
{
    Q_OBJECT
public:
    ~ResourceGeneratorProxy();
    Q_INVOKABLE void createResourceFileAsync(const QString &projectName = "share");
    Q_INVOKABLE std::optional<Utils::FilePath> createResourceFileSync(const QString &projectName = "share");

private:
    QFuture<void> m_future;

signals:
    void errorOccurred(const QString &error);
    void resourceFileCreated(const Utils::FilePath &filePath);
};

} // namespace QmlDesigner::DesignViewer
