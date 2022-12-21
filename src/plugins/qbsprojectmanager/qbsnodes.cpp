// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qbsnodes.h"

#include "qbsnodetreebuilder.h"
#include "qbsproject.h"
#include "qbsprojectmanagerconstants.h"
#include "qbsprojectmanagerplugin.h"
#include "qbssession.h"

#include <android/androidconstants.h>
#include <coreplugin/idocument.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtsupportconstants.h>
#include <resourceeditor/resourcenode.h>

#include <utils/algorithm.h>
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

namespace QbsProjectManager {
namespace Internal {

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

void QbsProductNode::build()
{
    QbsProjectManagerPlugin::buildNamedProduct(static_cast<QbsProject *>(getProject()),
                                               m_productData.value("full-display-name").toString());
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

bool QbsProductNode::isAggregated() const
{
    return m_productData.value("is-multiplexed").toBool()
            && m_productData.value("multiplex-configuration-id").toString().isEmpty();
}

const QList<const QbsProductNode*> QbsProductNode::aggregatedProducts() const
{
    if (!isAggregated())
        return {};
    const ProjectNode *parentNode = managingProject();
    QTC_ASSERT(parentNode != nullptr && parentNode != this, return {});

    QSet<QString> dependencies;
    for (const auto &a : m_productData.value("dependencies").toArray())
        dependencies << a.toString();

    QList<const QbsProductNode*> qbsProducts;
    parentNode->forEachProjectNode([&qbsProducts, dependencies](const ProjectNode *node) {
        if (const auto qbsChildNode = dynamic_cast<const QbsProductNode *>(node)) {
            if (dependencies.contains(qbsChildNode->fullDisplayName()))
                qbsProducts << qbsChildNode;
        }
    });
    return qbsProducts;
}

QVariant QbsProductNode::data(Id role) const
{
    if (role == Android::Constants::AndroidDeploySettingsFile) {
        for (const auto &a : m_productData.value("generated-artifacts").toArray()) {
            const QJsonObject artifact = a.toObject();
            if (artifact.value("file-tags").toArray().contains("qt_androiddeployqt_input"))
                return artifact.value("file-path").toString();
        }
        return {};
    }

    if (role == Android::Constants::AndroidSoLibPath) {
        QStringList ret{m_productData.value("build-directory").toString()};
        if (!isAggregated()) {
            forAllArtifacts(m_productData, ArtifactType::Generated,
                            [&ret](const QJsonObject &artifact) {
                if (artifact.value("file-tags").toArray().contains("dynamiclibrary"))
                    ret << QFileInfo(artifact.value("file-path").toString()).path();
            });
        } else {
            for (const auto &a : aggregatedProducts())
                ret += a->data(Android::Constants::AndroidSoLibPath).toStringList();
        }
        ret.removeDuplicates();
        return ret;
    }

    if (role == Android::Constants::AndroidManifest) {
        for (const auto &a : m_productData.value("generated-artifacts").toArray()) {
            const QJsonObject artifact = a.toObject();
            if (artifact.value("file-tags").toArray().contains("android.manifest_final"))
                return artifact.value("file-path").toString();
        }
        return {};
    }

    if (role == Android::Constants::AndroidApk)
        return m_productData.value("target-executable").toString();

    if (role == ProjectExplorer::Constants::QT_KEYWORDS_ENABLED)
        return m_productData.value("module-properties").toObject()
                .value("Qt.core.enableKeywords").toBool();

    if (role == Android::Constants::AndroidAbis) {
        // Try using qbs.architectures
        QStringList qbsAbis;
        QMap<QString, QString> archToAbi {
            {"armv7a", ProjectExplorer::Constants::ANDROID_ABI_ARMEABI_V7A},
            {"arm64", ProjectExplorer::Constants::ANDROID_ABI_ARM64_V8A},
            {"x86", ProjectExplorer::Constants::ANDROID_ABI_X86},
            {"x86_64", ProjectExplorer::Constants::ANDROID_ABI_X86_64}};
        for (const auto &a : m_productData.value("module-properties").toObject()
             .value(Constants::QBS_ARCHITECTURES).toArray()) {
            if (archToAbi.contains(a.toString()))
                qbsAbis << archToAbi[a.toString()];
        }
        if (!qbsAbis.empty())
            return qbsAbis;
        // Try using qbs.architecture
        QString architecture = m_productData.value("module-properties").toObject()
                .value(Constants::QBS_ARCHITECTURE).toString();
        if (archToAbi.contains(architecture))
            qbsAbis << archToAbi[architecture];
        return qbsAbis;
    }
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

} // namespace Internal
} // namespace QbsProjectManager
