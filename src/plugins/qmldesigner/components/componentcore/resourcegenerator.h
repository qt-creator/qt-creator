// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldesignercomponents_global.h>
#include <utils/filepath.h>
#include <utils/qtcprocess.h>

namespace QmlDesigner {
class ResourceGenerator : public QObject
{
    Q_OBJECT
public:
    ResourceGenerator(QObject *parent = nullptr);
    static void generateMenuEntry(QObject *parent);

    Q_INVOKABLE bool createQrc(const Utils::FilePath &qrcFilePath);
    Q_INVOKABLE std::optional<Utils::FilePath> createQrc(const QString &projectName = "share");

    Q_INVOKABLE bool createQmlrcWithPath(const Utils::FilePath &qmlrcFilePath);
    Q_INVOKABLE std::optional<Utils::FilePath> createQmlrcWithName(
        const QString &projectName = "share");
    Q_INVOKABLE void createQmlrcAsyncWithPath(const Utils::FilePath &qmlrcFilePath);
    Q_INVOKABLE void createQmlrcAsyncWithName(const QString &projectName = "share");

    Q_INVOKABLE void cancel();

private:
    Utils::Process m_rccProcess;
    Utils::FilePath m_qmlrcFilePath;

private:
    bool runRcc(const Utils::FilePath &qmlrcFilePath,
                const Utils::FilePath &qrcFilePath,
                const bool runAsync = false);

signals:
    void errorOccurred(const QString &error);
    void qmlrcCreated(const Utils::FilePath &filePath);
};

} // namespace QmlDesigner
