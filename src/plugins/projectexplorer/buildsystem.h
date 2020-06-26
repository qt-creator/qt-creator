/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "projectexplorer_export.h"

#include "buildtargetinfo.h"
#include "project.h"
#include "treescanner.h"

#include <QObject>

namespace ProjectExplorer {

class BuildConfiguration;
class Node;

// --------------------------------------------------------------------
// BuildSystem:
// --------------------------------------------------------------------

// Check buildsystem.md for more information
class PROJECTEXPLORER_EXPORT BuildSystem : public QObject
{
    Q_OBJECT

public:
    explicit BuildSystem(Target *target);
    explicit BuildSystem(BuildConfiguration *bc);
    ~BuildSystem() override;

    Project *project() const;
    Target *target() const;
    Kit *kit() const;
    BuildConfiguration *buildConfiguration() const;

    Utils::FilePath projectFilePath() const;
    Utils::FilePath projectDirectory() const;

    bool isWaitingForParse() const;

    void requestParse();
    void requestDelayedParse();
    void requestParseWithCustomDelay(int delayInMs = 1000);
    void cancelDelayedParseRequest();
    void setParseDelay(int delayInMs);
    int parseDelay() const;

    bool isParsing() const;
    bool hasParsingData() const;

    Utils::Environment activeParseEnvironment() const;

    virtual bool addFiles(Node *context, const QStringList &filePaths, QStringList *notAdded = nullptr);
    virtual RemovedFilesFromProject removeFiles(Node *context, const QStringList &filePaths,
                                                QStringList *notRemoved = nullptr);
    virtual bool deleteFiles(Node *context, const QStringList &filePaths);
    virtual bool canRenameFile(Node *context, const QString &filePath, const QString &newFilePath);
    virtual bool renameFile(Node *context, const QString &filePath, const QString &newFilePath);
    virtual bool addDependencies(Node *context, const QStringList &dependencies);
    virtual bool supportsAction(Node *context, ProjectAction action, const Node *node) const;

    virtual QStringList filesGeneratedFrom(const QString &sourceFile) const;
    virtual QVariant additionalData(Utils::Id id) const;

    void setDeploymentData(const DeploymentData &deploymentData);
    DeploymentData deploymentData() const;

    void setApplicationTargets(const QList<BuildTargetInfo> &appTargets);
    const QList<BuildTargetInfo> applicationTargets() const;
    BuildTargetInfo buildTarget(const QString &buildKey) const;

    void setRootProjectNode(std::unique_ptr<ProjectNode> &&root);

    class PROJECTEXPLORER_EXPORT ParseGuard
    {
        friend class BuildSystem;
        explicit ParseGuard(BuildSystem *p);

        void release();

    public:
        ParseGuard() = default;
        ~ParseGuard() { release(); }

        void markAsSuccess() const { m_success = true; }
        bool isSuccess() const { return m_success; }
        bool guardsProject() const { return m_buildSystem; }

        ParseGuard(const ParseGuard &other) = delete;
        ParseGuard &operator=(const ParseGuard &other) = delete;
        ParseGuard(ParseGuard &&other);
        ParseGuard &operator=(ParseGuard &&other);

    private:
        BuildSystem *m_buildSystem = nullptr;
        mutable bool m_success = false;
    };

    void emitBuildSystemUpdated();

    void setExtraData(const QString &buildKey, Utils::Id dataKey, const QVariant &data);
    QVariant extraData(const QString &buildKey, Utils::Id dataKey) const;

public:
    // FIXME: Make this private and the BuildSystem a friend
    ParseGuard guardParsingRun() { return ParseGuard(this); }

    QString disabledReason(const QString &buildKey) const;

    virtual void triggerParsing() = 0;

signals:
    void parsingStarted();
    void parsingFinished(bool success);
    void deploymentDataChanged();
    void applicationTargetsChanged();

protected:
    // Helper methods to manage parsing state and signalling
    // Call in GUI thread before the actual parsing starts
    void emitParsingStarted();
    // Call in GUI thread right after the actual parsing is done
    void emitParsingFinished(bool success);

private:
    void requestParseHelper(int delay); // request a (delayed!) parser run.

    class BuildSystemPrivate *d = nullptr;
};

} // namespace ProjectExplorer
