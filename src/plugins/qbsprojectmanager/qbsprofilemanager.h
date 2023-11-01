// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qbsprojectmanager_global.h"

#include <QList>
#include <QVariant>

namespace ProjectExplorer { class Kit; }

namespace QbsProjectManager {
namespace Internal {

QString toJSLiteral(const QVariant &val);
QVariant fromJSLiteral(const QString &str);

class QbsProfileManager : public QObject
{
    Q_OBJECT

public:
    QbsProfileManager();
    ~QbsProfileManager() override;

    static QbsProfileManager *instance();

    static QString ensureProfileForKit(const ProjectExplorer::Kit *k);
    static QString profileNameForKit(const ProjectExplorer::Kit *kit);
    static void updateProfileIfNecessary(const ProjectExplorer::Kit *kit);
    enum class QbsConfigOp { Get, Set, Unset, AddProfile };
    static QString runQbsConfig(QbsConfigOp op, const QString &key, const QVariant &value = {});

signals:
    void qbsProfilesUpdated();

private:
    void addProfileFromKit(const ProjectExplorer::Kit *k);
    void updateAllProfiles();

    void handleKitUpdate(ProjectExplorer::Kit *kit);
    void handleKitRemoval(ProjectExplorer::Kit *kit);

    QList<ProjectExplorer::Kit *> m_kitsToBeSetupForQbs;
};

} // namespace Internal
} // namespace QbsProjectManager
