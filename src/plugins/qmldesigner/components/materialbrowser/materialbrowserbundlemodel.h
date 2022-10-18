/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include <modelnode.h>
#include <qmlobjectnode.h>

#include <QAbstractListModel>
#include <QJsonObject>
#include <QObject>
#include <QPointer>

namespace QmlDesigner {

class BundleMaterial;
class BundleMaterialCategory;

namespace Internal {
class BundleImporter;
}

class MaterialBrowserBundleModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(bool matBundleExists MEMBER m_matBundleExists CONSTANT)
    Q_PROPERTY(bool isEmpty MEMBER m_isEmpty NOTIFY isEmptyChanged)
    Q_PROPERTY(bool hasQuick3DImport READ hasQuick3DImport WRITE setHasQuick3DImport NOTIFY hasQuick3DImportChanged)
    Q_PROPERTY(bool hasMaterialRoot READ hasMaterialRoot WRITE setHasMaterialRoot NOTIFY hasMaterialRootChanged)
    Q_PROPERTY(bool importerRunning MEMBER m_importerRunning NOTIFY importerRunningChanged)

public:
    MaterialBrowserBundleModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setSearchText(const QString &searchText);
    void updateImportedState(const QStringList &importedMats);

    bool hasQuick3DImport() const;
    void setHasQuick3DImport(bool b);

    bool hasMaterialRoot() const;
    void setHasMaterialRoot(bool b);

    Internal::BundleImporter *bundleImporter() const;

    void resetModel();

    Q_INVOKABLE void applyToSelected(QmlDesigner::BundleMaterial *mat, bool add = false);
    Q_INVOKABLE void addToProject(QmlDesigner::BundleMaterial *mat);
    Q_INVOKABLE void removeFromProject(QmlDesigner::BundleMaterial *mat);

signals:
    void isEmptyChanged();
    void hasQuick3DImportChanged();
    void hasMaterialRootChanged();
    void materialVisibleChanged();
    void applyToSelectedTriggered(QmlDesigner::BundleMaterial *mat, bool add = false);
    void bundleMaterialImported(const QmlDesigner::NodeMetaInfo &metaInfo);
    void bundleMaterialAboutToUnimport(const QmlDesigner::TypeName &type);
    void bundleMaterialUnimported(const QmlDesigner::NodeMetaInfo &metaInfo);
    void importerRunningChanged();

private:
    void loadMaterialBundle();
    bool isValidIndex(int idx) const;

    QString m_searchText;
    QList<BundleMaterialCategory *> m_bundleCategories;
    QJsonObject m_matBundleObj;
    Internal::BundleImporter *m_importer = nullptr;

    bool m_isEmpty = true;
    bool m_hasQuick3DImport = false;
    bool m_hasMaterialRoot = false;
    bool m_matBundleExists = false;
    bool m_probeMatBundleDir = false;
    bool m_importerRunning = false;
};

} // namespace QmlDesigner
