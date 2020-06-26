/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
    AddNewInformation addNewInformation(const QStringList &files, Node *context) const override;
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
