// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "useritemcategory.h"

#include "contentlibraryitem.h"

#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>

#include <QJsonArray>
#include <QJsonDocument>

namespace QmlDesigner {

UserItemCategory::UserItemCategory(const QString &title, const Utils::FilePath &bundlePath,
                                   const QString &bundleId)
    : UserCategory(title, bundlePath)
    , m_bundleId(bundleId)
{
}

void UserItemCategory::loadBundle(bool force)
{
    if (m_bundleLoaded && !force)
        return;

    // clean up
    qDeleteAll(m_items);
    m_items.clear();
    m_bundleLoaded = false;
    m_noMatch = false;
    m_bundleObj = {};

    m_bundlePath.ensureWritableDir();
    m_bundlePath.pathAppended("icons").ensureWritableDir();

    auto jsonFilePath = m_bundlePath.pathAppended(Constants::BUNDLE_JSON_FILENAME);
    if (!jsonFilePath.exists()) {
        QString jsonContent = "{\n";
        jsonContent += "    \"id\": \"" + m_bundleId + "\",\n";
        jsonContent += "    \"items\": []\n";
        jsonContent += "}";
        Utils::expected_str<qint64> res = jsonFilePath.writeFileContents(jsonContent.toLatin1());
        if (!res.has_value()) {
            qWarning() << __FUNCTION__ << res.error();
            setIsEmpty(true);
            emit itemsChanged();
            return;
        }
    }

    Utils::expected_str<QByteArray> jsonContents = jsonFilePath.fileContents();
    if (!jsonContents.has_value()) {
        qWarning() << __FUNCTION__ << jsonContents.error();
        setIsEmpty(true);
        emit itemsChanged();
        return;
    }

    QJsonDocument bundleJsonDoc = QJsonDocument::fromJson(jsonContents.value());
    if (bundleJsonDoc.isNull()) {
        qWarning() << __FUNCTION__ << "Invalid json file" << jsonFilePath;
        setIsEmpty(true);
        emit itemsChanged();
        return;
    }

    m_bundleObj = bundleJsonDoc.object();
    m_bundleObj["id"] = m_bundleId;

    // parse items
    auto compUtils = QmlDesignerPlugin::instance()->documentManager().generatedComponentUtils();
    QString typePrefix = compUtils.userBundleType(m_bundleId);
    const QJsonArray itemsArr = m_bundleObj.value("items").toArray();
    for (const QJsonValueConstRef &itemRef : itemsArr) {
        const QJsonObject itemObj = itemRef.toObject();

        QString name = itemObj.value("name").toString();
        QString qml = itemObj.value("qml").toString();
        TypeName type = QLatin1String("%1.%2").arg(typePrefix, qml.chopped(4)).toLatin1();
        QUrl icon = m_bundlePath.pathAppended(itemObj.value("icon").toString()).toUrl();
        QStringList files = itemObj.value("files").toVariant().toStringList();

        m_items.append(new ContentLibraryItem(this, name, qml, type, icon, files, m_bundleId));
    }

    m_sharedFiles.clear();
    const QJsonArray sharedFilesArr = m_bundleObj.value("sharedFiles").toArray();
    for (const QJsonValueConstRef &file : sharedFilesArr)
        m_sharedFiles.append(file.toString());

    m_bundleLoaded = true;
    setIsEmpty(m_items.isEmpty());
    emit itemsChanged();
}

void UserItemCategory::filter(const QString &searchText)
{
    bool noMatch = true;
    for (QObject *item : std::as_const(m_items)) {
        ContentLibraryItem *castedItem = qobject_cast<ContentLibraryItem *>(item);
        bool itemVisible = castedItem->filter(searchText);
        if (itemVisible)
            noMatch = false;
    }
    setNoMatch(noMatch);
}

QStringList UserItemCategory::sharedFiles() const
{
    return m_sharedFiles;
}

QJsonObject &UserItemCategory::bundleObjRef()
{
    return m_bundleObj;
}

} // namespace QmlDesigner
