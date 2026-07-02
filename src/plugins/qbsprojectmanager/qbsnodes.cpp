// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qbsnodes.h"

#include "qbsnodetreebuilder.h"
#include "qbsproject.h"
#include "qbsprojectmanagerconstants.h"
#include "qbsprojectmanagerplugin.h"
#include "qbssession.h"

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/fsengine/fileiconprovider.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QtDebug>
#include <QDir>
#include <QIcon>
#include <QJsonArray>

using namespace ProjectExplorer;
using namespace Utils;

// ----------------------------------------------------------------------
// Helpers:
// ----------------------------------------------------------------------

namespace QbsProjectManager::Internal {

const QbsProductNode *parentQbsProductNode(const ProjectExplorer::Node *node)
{
    for (; node; node = node->parentFolderNode()) {
        const auto prdNode = dynamic_cast<const QbsProductNode *>(node);
        if (prdNode)
            return prdNode;
    }
    return nullptr;
}

// --------------------------------------------------------------------
// QbsGroupNode:
// --------------------------------------------------------------------

QbsGroupNode::QbsGroupNode(const QJsonObject &grp) : ProjectNode(FilePath()), m_groupData(grp)
{
    setIcon(ProjectExplorer::Constants::FILEOVERLAY_GROUP);
    setDisplayName(grp.value("name").toString());
    setEnabled(grp.value("is-enabled").toBool());
}

FolderNode::AddNewInformation QbsGroupNode::addNewInformation(const FilePaths &files,
                                                              Node *context) const
{
    AddNewInformation info = ProjectNode::addNewInformation(files, context);
    if (context != this)
        --info.priority;
    return info;
}

QVariant QbsGroupNode::data(Id role) const
{
    if (role == ProjectExplorer::Constants::QT_KEYWORDS_ENABLED) {
        QJsonObject modProps = m_groupData.value("module-properties").toObject();
        if (modProps.isEmpty()) {
            const QbsProductNode * const prdNode = parentQbsProductNode(this);
            QTC_ASSERT(prdNode, return QVariant());
            modProps = prdNode->productData().value("module-properties").toObject();
        }
        return modProps.value("Qt.core.enableKeywords").toBool();
    }
    return QVariant();
}

// --------------------------------------------------------------------
// QbsProductNode:
// --------------------------------------------------------------------

QbsProductNode::QbsProductNode(const QJsonObject &prd) : ProjectNode(FilePath()), m_productData(prd)
{
    setIcon(DirectoryIcon(ProjectExplorer::Constants::FILEOVERLAY_PRODUCT));
    if (prd.value("is-runnable").toBool()) {
        setProductType(ProductType::App);
    } else {
        const QJsonArray type = prd.value("type").toArray();
        if (type.contains("dynamiclibrary") || type.contains("staticlibrary"))
            setProductType(ProductType::Lib);
        else
            setProductType(ProductType::Other);
    }
    setEnabled(prd.value("is-enabled").toBool());
    setDisplayName(prd.value("full-display-name").toString());
}

void QbsProductNode::build(BuildAction action)
{
    runStepsForNamedProducts(
        static_cast<QbsProject *>(getProject()),
        {m_productData.value("full-display-name").toString()},
        action);
}

QStringList QbsProductNode::targetApplications() const
{
    return QStringList{m_productData.value("target-executable").toString()};
}

QString QbsProductNode::fullDisplayName() const
{
    return m_productData.value("full-display-name").toString();
}

QString QbsProductNode::buildKey() const
{
    return getBuildKey(productData());
}

QString QbsProductNode::getBuildKey(const QJsonObject &product)
{
    return product.value("name").toString() + '.'
            + product.value("multiplex-configuration-id").toString();
}

QVariant QbsProductNode::data(Id role) const
{
    if (role == ProjectExplorer::Constants::QT_KEYWORDS_ENABLED)
        return m_productData.value("module-properties").toObject()
                .value("Qt.core.enableKeywords").toBool();

    return {};
}

QJsonObject QbsProductNode::mainGroup() const
{
    for (const QJsonValue &g : m_productData.value("groups").toArray()) {
        const QJsonObject grp = g.toObject();
        if (grp.value("name") == m_productData.value("name")
                && grp.value("location") == m_productData.value("location")) {
            return grp;
        }
    }
    return {};
}

// --------------------------------------------------------------------
// QbsProjectNode:
// --------------------------------------------------------------------

QbsProjectNode::QbsProjectNode(const QJsonObject &projectData)
    : ProjectNode(FilePath()), m_projectData(projectData)
{
    setIcon(DirectoryIcon(ProjectExplorer::Constants::FILEOVERLAY_QT));
    setDisplayName(projectData.value("name").toString());
}

void QbsProjectNode::build(ProjectExplorer::BuildAction action)
{
    QStringList toBuild;
    forAllProducts(m_projectData, [&toBuild](const QJsonObject &data) {
        toBuild << data.value("full-display-name").toString();
    });
    runStepsForNamedProducts(static_cast<QbsProject *>(getProject()), toBuild, action);
}

} // namespace QbsProjectManager::Internal
