// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <projectexplorer/taskhub.h>
#include <utils/id.h>

namespace QmlProjectManager {

class QmlProject;
class QmlProjectItem;
class QmlBuildSystem;

namespace QmlProjectExporter {

class FileGenerator : public QObject
{
    Q_OBJECT

public:
    static QAction *createMenuAction(QObject *parent, const QString &name, const Utils::Id &id);
    static void logIssue(
        ProjectExplorer::Task::TaskType type, const QString &text, const Utils::FilePath &file);

    FileGenerator(QmlBuildSystem *bs = nullptr);
    virtual ~FileGenerator() = default;

    virtual void updateMenuAction() = 0;
    virtual void updateProject(QmlProject *project) = 0;

    const QmlProject *qmlProject() const;
    const QmlBuildSystem *buildSystem() const;

    bool isEnabled() const;
    void setEnabled(bool enabled);

protected:
    void updateMenuAction(const Utils::Id &id, std::function<bool(void)> isEnabled);

private:
    bool m_enabled = false;
    QmlBuildSystem *m_buildSystem = nullptr;
};

} // namespace QmlProjectExporter.
} // namespace QmlProjectManager.
