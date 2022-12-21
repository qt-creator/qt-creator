// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include "buildinfo.h"
#include "kit.h"
#include "task.h"

#include <utils/guard.h>

#include <QWidget>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QGridLayout;
class QLabel;
class QPushButton;
class QSpacerItem;
QT_END_NAMESPACE

namespace Utils {
class DetailsWidget;
class PathChooser;
} // namespace Utils

namespace ProjectExplorer {
class BuildInfo;

namespace Internal {

class TargetSetupWidget : public QWidget
{
    Q_OBJECT

public:
    TargetSetupWidget(Kit *k, const Utils::FilePath &projectPath);

    Kit *kit() const;
    void clearKit();

    bool isKitSelected() const;
    void setKitSelected(bool b);

    void addBuildInfo(const BuildInfo &info, bool isImport);

    const QList<BuildInfo> selectedBuildInfoList() const;
    void setProjectPath(const Utils::FilePath &projectPath);
    void expandWidget();
    void update(const TasksGenerator &generator);

signals:
    void selectedToggled() const;

private:
    static const QList<BuildInfo> buildInfoList(const Kit *k, const Utils::FilePath &projectPath);

    bool hasSelectedBuildConfigurations() const;

    void toggleEnabled(bool enabled);
    void checkBoxToggled(QCheckBox *checkBox, bool b);
    void pathChanged(Utils::PathChooser *pathChooser);
    void targetCheckBoxToggled(bool b);
    void manageKit();

    void reportIssues(int index);
    QPair<Task::TaskType, QString> findIssues(const BuildInfo &info);
    void clear();
    void updateDefaultBuildDirectories();

    Kit *m_kit;
    Utils::FilePath m_projectPath;
    bool m_haveImported = false;
    Utils::DetailsWidget *m_detailsWidget;
    QPushButton *m_manageButton;
    QGridLayout *m_newBuildsLayout;

    struct BuildInfoStore {
        ~BuildInfoStore();
        BuildInfoStore() = default;
        BuildInfoStore(const BuildInfoStore &other) = delete;
        BuildInfoStore(BuildInfoStore &&other);
        BuildInfoStore &operator=(const BuildInfoStore &other) = delete;
        BuildInfoStore &operator=(BuildInfoStore &&other) = delete;

        BuildInfo buildInfo;
        QCheckBox *checkbox = nullptr;
        QLabel *label = nullptr;
        QLabel *issuesLabel = nullptr;
        Utils::PathChooser *pathChooser = nullptr;
        bool isEnabled = false;
        bool hasIssues = false;
        bool customBuildDir = false;
    };
    std::vector<BuildInfoStore> m_infoStore;

    Utils::Guard m_ignoreChanges;
    int m_selected = 0; // Number of selected "buildconfigurations"
};

} // namespace Internal
} // namespace ProjectExplorer
