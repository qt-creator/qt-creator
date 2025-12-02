// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidmanifestutils.h"

#include <QFile>
#include <QDomDocument>

namespace Android::Internal {

using namespace Utils;

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
    QDomElement permissionElem = manifest.firstChildElement(QLatin1String("uses-permission"));
    while (!permissionElem.isNull()) {
        data.permissions << permissionElem.attribute(QLatin1String("android:name"));
        permissionElem = permissionElem.nextSiblingElement(QLatin1String("uses-permission"));
    }
}

static void extractIconInfo(const QDomElement &manifest, AndroidManifestParser::ManifestData &data)
{
    QDomElement applicationElement = manifest.firstChildElement(QLatin1String("application"));
    if (!applicationElement.isNull() && applicationElement.hasAttribute(QLatin1String("android:icon"))) {
        QString appIconValue = applicationElement.attribute(QLatin1String("android:icon"));
        if (appIconValue.startsWith(QLatin1String("@drawable/"))) {
            data.iconName = appIconValue.mid(10); // Skip "@drawable/" prefix
            data.hasIcon = true;
        }
    }
}

Result<AndroidManifestParser::ManifestData> AndroidManifestParser::readManifest(const FilePath &manifestPath)
{
    auto docResult = loadManifestDocument(manifestPath);
    if (!docResult)
        return ResultError(docResult.error());

    ManifestData data;
    QDomElement manifest = docResult->documentElement();

    extractPlaceholderTags(manifest, data);
    extractPermissions(manifest, data);
    extractIconInfo(manifest, data);

    return data;
}

static void modifyApplicationAttributes(QDomElement &manifest,
                                        const AndroidManifestParser::ModifyParams &instructions)
{
    QDomElement applicationElement = manifest.firstChildElement(QLatin1String("application"));
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

    QDomElement permissionElem = manifest.firstChildElement(QLatin1String("uses-permission"));
    while (!permissionElem.isNull()) {
        QDomElement nextPermission = permissionElem.nextSiblingElement(QLatin1String("uses-permission"));
        QString permissionName = permissionElem.attribute(QLatin1String("android:name"));

        if (instructions.permissionsToKeep.contains(permissionName)) {
            permissionsToAdd.remove(permissionName);
            lastPermissionElem = permissionElem;
        } else {
            manifest.removeChild(permissionElem);
        }

        permissionElem = nextPermission;
    }

    QDomElement applicationElement = manifest.firstChildElement(QLatin1String("application"));
    for (const QString &permission : std::as_const(permissionsToAdd)) {
        QDomElement newPermission = doc.createElement(QLatin1String("uses-permission"));
        newPermission.setAttribute(QLatin1String("android:name"), permission);

        if (!lastPermissionElem.isNull()) {
            manifest.insertAfter(newPermission, lastPermissionElem);
            lastPermissionElem = newPermission;
        } else if (!applicationElement.isNull()) {
            manifest.insertBefore(newPermission, applicationElement);
        } else {
            manifest.appendChild(newPermission);
        }
    }

    QDomNodeList children = manifest.childNodes();
    for (int i = children.size() - 1; i >= 0; --i) {
        QDomNode child = children.at(i);
        if (child.isComment()) {
            QDomComment comment = child.toComment();
            QString commentText = comment.data().trimmed();

            if (commentText == QLatin1String("%%INSERT_PERMISSIONS")) {
                if (!instructions.writeDefaultPermissionsComment)
                    manifest.removeChild(child);
            } else if (commentText == QLatin1String("%%INSERT_FEATURES")) {
                if (!instructions.writeDefaultFeaturesComment)
                    manifest.removeChild(child);
            }
        }
    }
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

    QFile file(manifestPath.toFSPathString());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        return ResultError(QString("Cannot open manifest file for writing: %1")
                                      .arg(file.errorString()));

    file.write(doc.toString(4).toUtf8()); // Force 4 space indent
    return {};
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

} // namespace Android::Internal
