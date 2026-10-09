// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidmanifestutils.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/idocument.h>

#include <QFile>
#include <QDomDocument>

#ifdef WITH_TESTS
#include <utils/temporarydirectory.h>
#include <QTest>
#endif

namespace Android::Internal {

using namespace Utils;

static const QLatin1String keyApplication("application");
static const QLatin1String keyActivity("activity");
static const QLatin1String keyMetaData("meta-data");
static const QLatin1String keyUsesPermission("uses-permission");
static const QLatin1String keyAndroidName("android:name");
static const QLatin1String keyAndroidValue("android:value");
static const QLatin1String keyAndroidResource("android:resource");
static const QLatin1String keyAndroidIcon("android:icon");

static Result<QDomDocument> loadManifestDocument(const FilePath &manifestPath)
{
    QFile file(manifestPath.toFSPathString());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return ResultError(QString("Cannot open manifest file for reading: %1")
                                      .arg(file.errorString()));

    QDomDocument doc;
    QString errorMsg;
    int errorLine, errorColumn;
    if (!doc.setContent(&file, &errorMsg, &errorLine, &errorColumn))
        return ResultError(QString("XML parsing error at line %1, column %2: %3")
                                      .arg(errorLine).arg(errorColumn).arg(errorMsg));

    return doc;
}

static void extractPlaceholderTags(const QDomElement &manifest, AndroidManifestParser::ManifestData &data)
{
    QDomNodeList manifestChildren = manifest.childNodes();
    for (int i = 0; i < manifestChildren.size(); ++i) {
        const QDomNode &child = manifestChildren.at(i);
        if (child.isComment()) {
            QDomComment comment = child.toComment();
            QString commentText = comment.data().trimmed();
            if (commentText == QLatin1String("%%INSERT_PERMISSIONS"))
                data.hasDefaultPermissionsComment = true;
            else if (commentText == QLatin1String("%%INSERT_FEATURES"))
                data.hasDefaultFeaturesComment = true;
        }
    }
}

static void extractPermissions(const QDomElement &manifest, AndroidManifestParser::ManifestData &data)
{
    QDomElement permissionElem = manifest.firstChildElement(keyUsesPermission);
    while (!permissionElem.isNull()) {
        const QString name = permissionElem.attribute(keyAndroidName);
        if (!name.isEmpty()) {
            QMap<QString, QString> attributes;
            const QDomNamedNodeMap attrMap = permissionElem.attributes();
            for (int i = 0; i < attrMap.count(); ++i) {
                const QDomAttr attr = attrMap.item(i).toAttr();
                if (attr.name() != keyAndroidName)
                    attributes.insert(attr.name(), attr.value());
            }
            data.permissions.insert(name, attributes);
        }
        permissionElem = permissionElem.nextSiblingElement(keyUsesPermission);
    }
}

static void extractIconValue(const QDomElement &manifest, AndroidManifestParser::ManifestData &data)
{
    const QDomElement application = manifest.firstChildElement(keyApplication);
    if (!application.isNull())
        data.iconValue = application.attribute(keyAndroidIcon);
}

Result<AndroidManifestParser::ManifestData> AndroidManifestParser::readManifest(const FilePath &manifestPath)
{
    if (!manifestPath.isReadableFile())
        return ResultError("Could not read manifest file");

    auto docResult = loadManifestDocument(manifestPath);
    if (!docResult)
        return ResultError(docResult.error());

    ManifestData data;
    QDomElement manifest = docResult->documentElement();

    extractPlaceholderTags(manifest, data);
    extractPermissions(manifest, data);
    extractIconValue(manifest, data);

    return data;
}

static void modifyApplicationAttributes(QDomElement &manifest,
                                        const AndroidManifestParser::ModifyParams &instructions)
{
    QDomElement applicationElement = manifest.firstChildElement(keyApplication);
    if (!applicationElement.isNull()) {
        for (int i = 0; i < instructions.applicationKeys.size(); ++i) {
            applicationElement.setAttribute(instructions.applicationKeys.at(i),
                                            instructions.applicationValues.at(i));
        }
        for (const QString &key : instructions.applicationKeysToRemove) {
            applicationElement.removeAttribute(key);
        }
    }
}

static void modifyPermissions(QDomDocument &doc, QDomElement &manifest,
                              const AndroidManifestParser::ModifyParams &instructions)
{
    QSet<QString> permissionsToAdd = instructions.permissionsToKeep;
    QDomElement lastPermissionElem;

    QDomElement permissionElem = manifest.firstChildElement(keyUsesPermission);
    while (!permissionElem.isNull()) {
        QDomElement nextPermission = permissionElem.nextSiblingElement(keyUsesPermission);
        QString permissionName = permissionElem.attribute(keyAndroidName);

        if (instructions.permissionsToKeep.contains(permissionName)) {
            permissionsToAdd.remove(permissionName);
            lastPermissionElem = permissionElem;
        } else {
            manifest.removeChild(permissionElem);
        }

        permissionElem = nextPermission;
    }

    QDomElement applicationElement = manifest.firstChildElement(keyApplication);
    for (const QString &permission : std::as_const(permissionsToAdd)) {
        QDomElement newPermission = doc.createElement(keyUsesPermission);
        newPermission.setAttribute(keyAndroidName, permission);

        if (!lastPermissionElem.isNull()) {
            manifest.insertAfter(newPermission, lastPermissionElem);
            lastPermissionElem = newPermission;
        } else if (!applicationElement.isNull()) {
            manifest.insertBefore(newPermission, applicationElement);
        } else {
            manifest.appendChild(newPermission);
        }
    }

    bool hasPermissionsComment = false;
    bool hasFeaturesComment = false;

    QDomNodeList children = manifest.childNodes();
    for (int i = children.size() - 1; i >= 0; --i) {
        QDomNode child = children.at(i);
        if (child.isComment()) {
            QDomComment comment = child.toComment();
            QString commentText = comment.data().trimmed();

            if (commentText == QLatin1String("%%INSERT_PERMISSIONS")) {
                if (!instructions.writeDefaultPermissionsComment)
                    manifest.removeChild(child);
                else
                    hasPermissionsComment = true;
            } else if (commentText == QLatin1String("%%INSERT_FEATURES")) {
                if (!instructions.writeDefaultFeaturesComment)
                    manifest.removeChild(child);
                else
                    hasFeaturesComment = true;
            }
        }
    }

    if (instructions.writeDefaultPermissionsComment && !hasPermissionsComment) {
        QDomComment permComment = doc.createComment(QLatin1String("%%INSERT_PERMISSIONS"));
        if (!applicationElement.isNull())
            manifest.insertBefore(permComment, applicationElement);
        else
            manifest.appendChild(permComment);
    }
    if (instructions.writeDefaultFeaturesComment && !hasFeaturesComment) {
        QDomComment featComment = doc.createComment(QLatin1String("%%INSERT_FEATURES"));
        if (!applicationElement.isNull())
            manifest.insertBefore(featComment, applicationElement);
        else
            manifest.appendChild(featComment);
    }
}

static Result<void> modifyActivityMetaData(QDomDocument &doc, QDomElement &manifest,
                                           const AndroidManifestParser::ModifyParams &instructions)
{
    QDomElement application = manifest.firstChildElement(keyApplication);
    if (application.isNull())
        return ResultError("No application element found in manifest");

    QDomElement activity = application.firstChildElement(keyActivity);
    if (activity.isNull())
        return ResultError("No activity element found in manifest");

    QDomElement metaData = activity.firstChildElement(keyMetaData);
    while (!metaData.isNull() && metaData.attribute(keyAndroidName) != instructions.activityMetaDataName)
        metaData = metaData.nextSiblingElement(keyMetaData);

    const QString &value = instructions.activityMetaDataValue;
    if (value.isEmpty()) {
        if (!metaData.isNull())
            activity.removeChild(metaData);
    } else {
        if (metaData.isNull()) {
            metaData = doc.createElement(keyMetaData);
            metaData.setAttribute(keyAndroidName, instructions.activityMetaDataName);
            activity.appendChild(metaData);
        }
        const bool isResource = value.startsWith(QLatin1Char('@'));
        metaData.removeAttribute(isResource ? keyAndroidValue : keyAndroidResource);
        metaData.setAttribute(isResource ? keyAndroidResource : keyAndroidValue, value);
    }
    return {};
}

static Result<void> saveDocument(const FilePath &manifestPath, const QDomDocument &doc)
{
    Core::DocumentManager::expectFileChange(manifestPath);
    QScopeGuard unexpect([&] { Core::DocumentManager::unexpectFileChange(manifestPath); });
    QFile file(manifestPath.toFSPathString());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        return ResultError(QString("Cannot open manifest file for writing: %1")
                               .arg(file.errorString()));
    file.write(doc.toString(4).toUtf8());
    file.close();

    const QList<Core::IEditor *> editors = Core::DocumentModel::editorsForFilePath(manifestPath);
    for (Core::IEditor *editor : editors) {
        if (Core::IDocument *doc = editor->document())
            doc->reload(Core::IDocument::FlagReload, Core::IDocument::TypeContents);
    }
    return {};
}

Result<void>
AndroidManifestParser::processAndWriteManifest(const FilePath &manifestPath,
                                               const ModifyParams &instructions)
{
    auto docResult = loadManifestDocument(manifestPath);
    if (!docResult)
        return ResultError(docResult.error());

    QDomDocument doc = *docResult;
    QDomElement manifest = doc.documentElement();

    if (instructions.shouldModifyApplication)
        modifyApplicationAttributes(manifest, instructions);

    if (instructions.shouldModifyPermissions)
        modifyPermissions(doc, manifest, instructions);

    if (instructions.shouldModifyActivityMetaData) {
        auto result = modifyActivityMetaData(doc, manifest, instructions);
        if (!result)
            return result;
    }

    return saveDocument(manifestPath, doc);
}

Result<void> updateManifestApplicationAttribute(const FilePath &manifestPath,
                                                const QString &attributeKey,
                                                const QString &attributeValue)
{
    AndroidManifestParser::ModifyParams instructions;
    instructions.shouldModifyApplication = true;

    if (attributeValue.isEmpty()) {
        instructions.applicationKeysToRemove << attributeKey;
    } else {
        instructions.applicationKeys << attributeKey;
        instructions.applicationValues << attributeValue;
    }

    return AndroidManifestParser::processAndWriteManifest(manifestPath, instructions);
}

Result<void> updateManifestPermissions(const FilePath &manifestPath, const QStringList &permissions,
                                       bool includeDefaultPermissions, bool includeDefaultFeatures)
{
    AndroidManifestParser::ModifyParams instructions;
    instructions.shouldModifyPermissions = true;
    instructions.permissionsToKeep = QSet<QString>(permissions.begin(), permissions.end());
    instructions.writeDefaultPermissionsComment = includeDefaultPermissions;
    instructions.writeDefaultFeaturesComment = includeDefaultFeatures;

    return AndroidManifestParser::processAndWriteManifest(manifestPath, instructions);
}

Result<QString> readManifestActivityMetaData(const FilePath &manifestPath,
                                             const QString &metaDataName)
{
    auto docResult = loadManifestDocument(manifestPath);
    if (!docResult)
        return ResultError(docResult.error());

    QDomElement manifest = docResult->documentElement();
    QDomElement application = manifest.firstChildElement(keyApplication);
    if (application.isNull())
        return ResultError("No application element found in manifest");

    QDomElement activity = application.firstChildElement(keyActivity);
    while (!activity.isNull()) {
        QDomElement metaData = activity.firstChildElement(keyMetaData);
        while (!metaData.isNull()) {
            if (metaData.attribute(keyAndroidName) == metaDataName)
                return metaData.attribute(keyAndroidValue);
            metaData = metaData.nextSiblingElement(keyMetaData);
        }
        activity = activity.nextSiblingElement(keyActivity);
    }

    return QString();
}

Result<void> updateManifestActivityMetaData(const FilePath &manifestPath,
                                            const QString &metaDataName,
                                            const QString &metaDataValue)
{
    AndroidManifestParser::ModifyParams instructions;
    instructions.shouldModifyActivityMetaData = true;
    instructions.activityMetaDataName = metaDataName;
    instructions.activityMetaDataValue = metaDataValue;
    return AndroidManifestParser::processAndWriteManifest(manifestPath, instructions);
}

Result<void> updateManifestPermissionAttributes(const FilePath &manifestPath,
                                                const QString &permission,
                                                const QMap<QString, QString> &attributes)
{
    auto docResult = loadManifestDocument(manifestPath);
    if (!docResult)
        return ResultError(docResult.error());

    QDomDocument doc = *docResult;
    QDomElement manifest = doc.documentElement();

    QDomElement permissionElem = manifest.firstChildElement(keyUsesPermission);
    while (!permissionElem.isNull()) {
        if (permissionElem.attribute(keyAndroidName) == permission) {
            const QDomNamedNodeMap attrMap = permissionElem.attributes();
            QStringList attrNames;
            for (int i = 0; i < attrMap.count(); ++i)
                attrNames << attrMap.item(i).toAttr().name();
            for (const QString &name : std::as_const(attrNames)) {
                if (name != keyAndroidName)
                    permissionElem.removeAttribute(name);
            }
            for (auto it = attributes.cbegin(); it != attributes.cend(); ++it)
                permissionElem.setAttribute(it.key(), it.value());
            break;
        }
        permissionElem = permissionElem.nextSiblingElement(keyUsesPermission);
    }

    if (permissionElem.isNull())
        return ResultError(QString("Permission '%1' not found in manifest").arg(permission));

    return saveDocument(manifestPath, doc);
}

} // namespace Android::Internal

#ifdef WITH_TESTS

namespace Android::Internal {

class AndroidManifestUtilsTest final : public QObject
{
    Q_OBJECT

private slots:
    void testIconValueRoundTrip()
    {
        TemporaryDirectory dir("androidmanifest-XXXXXX");
        const FilePath manifest = dir.filePath("AndroidManifest.xml");
        QVERIFY(manifest.writeFileContents(
            "<?xml version=\"1.0\"?>\n"
            "<manifest xmlns:android=\"http://schemas.android.com/apk/res/android\">\n"
            "  <application android:icon=\"@mipmap/icon\"/>\n"
            "</manifest>\n"));

        Result<AndroidManifestParser::ManifestData> data
            = AndroidManifestParser::readManifest(manifest);
        QVERIFY(data);
        QCOMPARE(data->iconValue, QString("@mipmap/icon"));

        QVERIFY(updateManifestApplicationAttribute(manifest, "android:icon", "@drawable/other"));
        data = AndroidManifestParser::readManifest(manifest);
        QVERIFY(data);
        QCOMPARE(data->iconValue, QString("@drawable/other"));

        QVERIFY(updateManifestApplicationAttribute(manifest, "android:icon", {}));
        data = AndroidManifestParser::readManifest(manifest);
        QVERIFY(data);
        QVERIFY(data->iconValue.isEmpty());
    }
};

QObject *createAndroidManifestUtilsTest()
{
    return new AndroidManifestUtilsTest;
}

} // namespace Android::Internal

#include "androidmanifestutils.moc"

#endif // WITH_TESTS
