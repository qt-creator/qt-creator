// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "contentlibraryeffectsmodel.h"

#include "contentlibrarybundleimporter.h"
#include "contentlibraryeffect.h"
#include "contentlibraryeffectscategory.h"
#include "contentlibrarywidget.h"

#include <qmldesignerplugin.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/hostosinfo.h>

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUrl>

namespace QmlDesigner {

ContentLibraryEffectsModel::ContentLibraryEffectsModel(ContentLibraryWidget *parent)
    : QAbstractListModel(parent)
    , m_widget(parent)
{
}

int ContentLibraryEffectsModel::rowCount(const QModelIndex &) const
{
    return m_bundleCategories.size();
}

QVariant ContentLibraryEffectsModel::data(const QModelIndex &index, int role) const
{
    QTC_ASSERT(index.isValid() && index.row() < m_bundleCategories.count(), return {});
    QTC_ASSERT(roleNames().contains(role), return {});

    return m_bundleCategories.at(index.row())->property(roleNames().value(role));
}

bool ContentLibraryEffectsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || !roleNames().contains(role))
        return false;

    QByteArray roleName = roleNames().value(role);
    ContentLibraryEffectsCategory *bundleCategory = m_bundleCategories.at(index.row());
    QVariant currValue = bundleCategory->property(roleName);

    if (currValue != value) {
        bundleCategory->setProperty(roleName, value);

        emit dataChanged(index, index, {role});
        return true;
    }

    return false;
}

bool ContentLibraryEffectsModel::isValidIndex(int idx) const
{
    return idx > -1 && idx < rowCount();
}

QString ContentLibraryEffectsModel::bundleId() const
{
    return m_bundleId;
}

void ContentLibraryEffectsModel::updateIsEmpty()
{
    bool anyCatVisible = Utils::anyOf(m_bundleCategories, [&](ContentLibraryEffectsCategory *cat) {
        return cat->visible();
    });

    bool newEmpty = !anyCatVisible || m_bundleCategories.isEmpty() || !hasRequiredQuick3DImport();

    if (newEmpty != m_isEmpty) {
        m_isEmpty = newEmpty;
        emit isEmptyChanged();
    }
}

QHash<int, QByteArray> ContentLibraryEffectsModel::roleNames() const
{
    static const QHash<int, QByteArray> roles {
        {Qt::UserRole + 1, "bundleCategoryName"},
        {Qt::UserRole + 2, "bundleCategoryVisible"},
        {Qt::UserRole + 3, "bundleCategoryExpanded"},
        {Qt::UserRole + 4, "bundleCategoryItems"}
    };
    return roles;
}

void ContentLibraryEffectsModel::loadBundle()
{
    if (m_bundleExists || m_probeBundleDir)
        return;

    QDir bundleDir;

    if (!qEnvironmentVariable("EFFECT_BUNDLE_PATH").isEmpty())
        bundleDir.setPath(qEnvironmentVariable("EFFECT_BUNDLE_PATH"));
    else if (Utils::HostOsInfo::isMacHost())
        bundleDir.setPath(QCoreApplication::applicationDirPath() + "/../Resources/effect_bundle");

    // search for bundleDir from exec dir and up
    if (bundleDir.dirName() == ".") {
        m_probeBundleDir = true; // probe only once
        bundleDir.setPath(QCoreApplication::applicationDirPath());
        while (!bundleDir.cd("effect_bundle") && bundleDir.cdUp())
            ; // do nothing

        if (bundleDir.dirName() != "effect_bundle") // bundlePathDir not found
            return;
    }

    QString bundlePath = bundleDir.filePath("effect_bundle.json");

    if (m_bundleObj.isEmpty()) {
        QFile propsFile(bundlePath);

        if (!propsFile.open(QIODevice::ReadOnly)) {
            qWarning("Couldn't open effect_bundle.json");
            return;
        }

        QJsonDocument bundleJsonDoc = QJsonDocument::fromJson(propsFile.readAll());
        if (bundleJsonDoc.isNull()) {
            qWarning("Invalid effect_bundle.json file");
            return;
        } else {
            m_bundleObj = bundleJsonDoc.object();
        }
    }

    auto compUtils = QmlDesignerPlugin::instance()->documentManager().generatedComponentUtils();
    QString bundleType = compUtils.effectsBundleType();
    m_bundleId = compUtils.effectsBundleId();

    const QJsonObject catsObj = m_bundleObj.value("categories").toObject();
    const QStringList categories = catsObj.keys();
    for (const QString &cat : categories) {
        auto category = new ContentLibraryEffectsCategory(this, cat);

        const QJsonObject itemsObj = catsObj.value(cat).toObject();
        const QStringList items = itemsObj.keys();
        for (const QString &item : items) {
            const QJsonObject itemObj = itemsObj.value(item).toObject();

            QStringList files;
            const QJsonArray assetsArr = itemObj.value("files").toArray();
            for (const QJsonValueConstRef &asset : assetsArr)
                files.append(asset.toString());

            QUrl icon = QUrl::fromLocalFile(bundleDir.filePath(itemObj.value("icon").toString()));
            QString qml = itemObj.value("qml").toString();
            TypeName type = QLatin1String("%1.%2")
                                .arg(bundleType, qml.chopped(4)).toLatin1(); // chopped(4): remove .qml

            auto bundleItem = new ContentLibraryEffect(category, item, qml, type, icon, files);

            category->addBundleItem(bundleItem);
        }
        m_bundleCategories.append(category);
    }

    m_bundleSharedFiles.clear();
    const QJsonArray sharedFilesArr = m_bundleObj.value("sharedFiles").toArray();
    for (const QJsonValueConstRef &file : sharedFilesArr)
        m_bundleSharedFiles.append(file.toString());

    resetModel();
    updateIsEmpty();
    m_bundlePath = bundleDir.path();
    m_bundleExists = true;
    emit bundleExistsChanged();
}

bool ContentLibraryEffectsModel::hasRequiredQuick3DImport() const
{
    return m_widget->hasQuick3DImport() && m_quick3dMajorVersion == 6 && m_quick3dMinorVersion >= 4;
}

bool ContentLibraryEffectsModel::bundleExists() const
{
    return m_bundleExists;
}

void ContentLibraryEffectsModel::setSearchText(const QString &searchText)
{
    QString lowerSearchText = searchText.toLower();

    if (m_searchText == lowerSearchText)
        return;

    m_searchText = lowerSearchText;

    for (int i = 0; i < m_bundleCategories.size(); ++i) {
        ContentLibraryEffectsCategory *cat = m_bundleCategories.at(i);
        bool catVisibilityChanged = cat->filter(m_searchText);
        if (catVisibilityChanged)
            emit dataChanged(index(i), index(i), {roleNames().keys("bundleCategoryVisible")});
    }

    updateIsEmpty();
}

void ContentLibraryEffectsModel::updateImportedState(const QStringList &importedItems)
{
    bool changed = false;
    for (ContentLibraryEffectsCategory *cat : std::as_const(m_bundleCategories))
        changed |= cat->updateImportedState(importedItems);

    if (changed)
        resetModel();
}

void ContentLibraryEffectsModel::setQuick3DImportVersion(int major, int minor)
{
    bool oldRequiredImport = hasRequiredQuick3DImport();

    m_quick3dMajorVersion = major;
    m_quick3dMinorVersion = minor;

    bool newRequiredImport = hasRequiredQuick3DImport();

    if (oldRequiredImport == newRequiredImport)
        return;

    emit hasRequiredQuick3DImportChanged();

    updateIsEmpty();
}

void ContentLibraryEffectsModel::resetModel()
{
    beginResetModel();
    endResetModel();
}

void ContentLibraryEffectsModel::addInstance(ContentLibraryEffect *bundleItem)
{
    QString err = m_widget->importer()->importComponent(m_bundlePath, bundleItem->type(),
                                                        bundleItem->qml(),
                                                        bundleItem->files() + m_bundleSharedFiles);

    if (err.isEmpty())
        m_widget->setImporterRunning(true);
    else
        qWarning() << __FUNCTION__ << err;
}

void ContentLibraryEffectsModel::removeFromProject(ContentLibraryEffect *bundleItem)
{
    QString err = m_widget->importer()->unimportComponent(bundleItem->type(), bundleItem->qml());

    if (err.isEmpty())
        m_widget->setImporterRunning(true);
    else
        qWarning() << __FUNCTION__ << err;
}

} // namespace QmlDesigner
