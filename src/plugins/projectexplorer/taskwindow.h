// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/ioutputpane.h>

#include <memory>

QT_BEGIN_NAMESPACE
class QAction;
class QModelIndex;
class QPoint;
QT_END_NAMESPACE

namespace ProjectExplorer {
class TaskCategory;
class TaskHub;
class Task;

namespace Internal {
class TaskWindowPrivate;

// Show issues (warnings or errors) and open the editor on click.
class TaskWindow final : public Core::IOutputPane
{
    Q_OBJECT

public:
    TaskWindow();
    ~TaskWindow() override;

    void delayedInitialization();

    int taskCount(Utils::Id category = Utils::Id()) const;
    int warningTaskCount(Utils::Id category = Utils::Id()) const;
    int errorTaskCount(Utils::Id category = Utils::Id()) const;

    // IOutputPane
    QWidget *outputWidget(QWidget *) override;
    QList<QWidget *> toolBarWidgets() const override;

    void clearContents() override;
    void visibilityChanged(bool visible) override;

    bool canFocus() const override;
    bool hasFocus() const override;
    void setFocus() override;

    bool canNavigate() const override;
    bool canNext() const override;
    bool canPrevious() const override;
    void goToNext() override;
    void goToPrev() override;

signals:
    void tasksChanged();

private:
    void updateFilter() override;

    void addCategory(const TaskCategory &category);
    void addTask(const ProjectExplorer::Task &task);
    void removeTask(const ProjectExplorer::Task &task);
    void updatedTaskFileName(const Task &task, const QString &fileName);
    void updatedTaskLineNumber(const Task &task, int line);
    void showTask(const Task &task);
    void openTask(const Task &task);
    void clearTasks(Utils::Id categoryId);
    void setCategoryVisibility(Utils::Id categoryId, bool visible);
    void saveSettings();
    void loadSettings();

    void triggerDefaultHandler(const QModelIndex &index);
    void setShowWarnings(bool);
    void updateCategoriesMenu();

    int sizeHintForColumn(int column) const;

    const std::unique_ptr<TaskWindowPrivate> d;
};

} // namespace Internal
} // namespace ProjectExplorer
