// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/projectnodes.h>

#include <QJsonObject>

namespace QbsProjectManager {
namespace Internal {

class QbsProject;
class QbsBuildSystem;

class QbsGroupNode : public ProjectExplorer::ProjectNode
{
public:
    QbsGroupNode(const QJsonObject &grp);

    bool showInSimpleTree() const final { return false; }
    QJsonObject groupData() const { return m_groupData; }

private:
    friend class QbsBuildSystem;
    AddNewInformation addNewInformation(const Utils::FilePaths &files, Node *context) const override;
    QVariant data(Utils::Id role) const override;

    const QJsonObject m_groupData;
};

class QbsProductNode : public ProjectExplorer::ProjectNode
{
public:
    explicit QbsProductNode(const QJsonObject &prd);

    void build() override;
    QStringList targetApplications() const override;

    QString fullDisplayName() const;
    QString buildKey() const override;

    static QString getBuildKey(const QJsonObject &product);

    bool isAggregated() const;
    const QList<const QbsProductNode*> aggregatedProducts() const;

    const QJsonObject productData() const { return m_productData; }
    QJsonObject mainGroup() const;
    QVariant data(Utils::Id role) const override;

private:
    const QJsonObject m_productData;
};

class QbsProjectNode : public ProjectExplorer::ProjectNode
{
public:
    explicit QbsProjectNode(const QJsonObject &projectData);

    const QJsonObject projectData() const { return m_projectData; }

private:
    const QJsonObject m_projectData;
};

const QbsProductNode *parentQbsProductNode(const ProjectExplorer::Node *node);

} // namespace Internal
} // namespace QbsProjectManager
