// Copyright (C) 2022 the Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/algorithm.h>

#include <QDomDocument>
#include <QList>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QVersionNumber>

#include <optional>

Q_DECLARE_LOGGING_CATEGORY(updateLog)

struct Update
{
    QString name;
    QString version;

    bool operator==(const Update &other) const
    {
        return other.name == name && other.version == version;
    };
};

QList<Update> availableUpdates(const QString &updateXml)
{
    QDomDocument document;
    document.setContent(updateXml);
    if (document.isNull() || !document.firstChildElement().hasChildNodes())
        return {};
    QList<Update> result;
    const QDomNodeList updates = document.firstChildElement().elementsByTagName("update");
    for (int i = 0; i < updates.size(); ++i) {
        const QDomNode node = updates.item(i);
        if (node.isElement()) {
            const QDomElement element = node.toElement();
            if (element.hasAttribute("name"))
                result.append({element.attribute("name"), element.attribute("version")});
        }
    }
    return result;
}

struct QtPackage
{
    QString displayName;
    QVersionNumber version;
    bool installed;
    bool isPrerelease = false;

    bool operator==(const QtPackage &other) const
    {
        return other.installed == installed && other.isPrerelease == isPrerelease
               && other.version == version && other.displayName == displayName;
    }
};

QList<QtPackage> availableQtPackages(const QString &packageXml)
{
    QDomDocument document;
    document.setContent(packageXml);
    if (document.isNull() || !document.firstChildElement().hasChildNodes())
        return {};
    QList<QtPackage> result;
    const QDomNodeList packages = document.firstChildElement().elementsByTagName("package");
    for (int i = 0; i < packages.size(); ++i) {
        const QDomNode node = packages.item(i);
        if (node.isElement()) {
            const QDomElement element = node.toElement();
            if (element.hasAttribute("displayname") && element.hasAttribute("name")
                && element.hasAttribute("version")) {
                QtPackage package{element.attribute("displayname"),
                                  QVersionNumber::fromString(element.attribute("version")),
                                  element.hasAttribute("installedVersion")};
                // Heuristic: Prerelease if the name is not "Qt x.y.z"
                // (prereleases are named "Qt x.y.z-alpha" etc)
                package.isPrerelease = package.displayName
                                       != QString("Qt %1").arg(package.version.toString());
                result.append(package);
            }
        }
    }
    std::sort(result.begin(), result.end(), [](const QtPackage &p1, const QtPackage &p2) {
        return p1.version > p2.version;
    });
    return result;
}

// Expects packages to be sorted, high version first.
std::optional<QtPackage> highestInstalledQt(const QList<QtPackage> &packages)
{
    const auto highestInstalledIt = std::find_if(packages.cbegin(),
                                                 packages.cend(),
                                                 [](const QtPackage &p) { return p.installed; });
    if (highestInstalledIt == packages.cend()) // Qt not installed
        return {};
    return *highestInstalledIt;
}

// Expects packages to be sorted, high version first.
std::optional<QtPackage> qtToNagAbout(const QList<QtPackage> &allPackages,
                                        QVersionNumber *highestSeen)
{
    // Filter out any Qt prereleases
    const QList<QtPackage> packages = Utils::filtered(allPackages, [](const QtPackage &p) {
        return !p.isPrerelease;
    });
    if (packages.isEmpty())
        return {};
    const QtPackage highest = packages.constFirst();
    qCDebug(updateLog) << "Highest available (non-prerelease) Qt:" << highest.version;
    qCDebug(updateLog) << "Highest previously seen (non-prerelease) Qt:" << *highestSeen;
    // if the highestSeen version is null, we don't know if the Qt version is new, and better don't nag
    const bool isNew = !highestSeen->isNull() && highest.version > *highestSeen;
    if (highestSeen->isNull() || isNew)
        *highestSeen = highest.version;
    if (!isNew)
        return {};
    const std::optional<QtPackage> highestInstalled = highestInstalledQt(packages);
    qCDebug(updateLog) << "Highest installed Qt:"
                       << qPrintable(highestInstalled ? highestInstalled->version.toString()
                                                      : QString("none"));
    if (!highestInstalled) // don't nag if no Qt is installed at all
        return {};
    if (highestInstalled->version == highest.version)
        return {};
    return highest;
}

Q_DECLARE_METATYPE(Update)
Q_DECLARE_METATYPE(QtPackage)
