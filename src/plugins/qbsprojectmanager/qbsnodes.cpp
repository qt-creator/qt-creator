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

#include "qbsnodes.h"

#include "qbsnodetreebuilder.h"
#include "qbsproject.h"
#include "qbsprojectmanagerconstants.h"
#include "qbsprojectmanagerplugin.h"
#include "qbssession.h"

#include <android/androidconstants.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/idocument.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtsupportconstants.h>
#include <resourceeditor/resourcenode.h>
#include <utils/algorithm.h>
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
    static QIcon groupIcon = QIcon(QString(ProjectExplorer::Constants::FILEOVERLAY_GROUP));
    setIcon(groupIcon);
    setDisplayName(grp.value("name").toString());
    setEnabled(grp.value("is-enabled").toBool());
}

FolderNode::AddNewInformation QbsGroupNode::addNewInformation(const QStringList &files,
                                                              Node *context) const
{
    AddNewInformation info = ProjectNode::addNewInformation(files, context);
    if (context != this)
        --info.priority;
    return info;
}

QVariant QbsGroupNode::data(Core::Id role) const
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
    static QIcon productIcon = Core::FileIconProvider::directoryIcon(
                ProjectExplorer::Constants::FILEOVERLAY_PRODUCT);
    setIcon(productIcon);
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

QVariant QbsProductNode::data(Core::Id role) const
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
        forAllArtifacts(m_productData, ArtifactType::Generated, [&ret](const QJsonObject &artifact) {
            if (artifact.value("file-tags").toArray().contains("dynamiclibrary"))
                ret << QFileInfo(artifact.value("file-path").toString()).path();
        });
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
    static QIcon projectIcon = Core::FileIconProvider::directoryIcon(
                ProjectExplorer::Constants::FILEOVERLAY_QT);
    setIcon(projectIcon);
    setDisplayName(projectData.value("name").toString());
}

} // namespace Internal
} // namespace QbsProjectManager
