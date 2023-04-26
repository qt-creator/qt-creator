// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "modulescanner.h"

#ifdef QDS_HAS_QMLPRIVATE
#include <private/qqmldirparser_p.h>
#endif

#include <QDirIterator>
#include <QFile>
#include <QHash>

namespace QmlDesigner {

namespace {

#ifdef QDS_HAS_QMLPRIVATE
std::optional<QString> contentAsQString(const QString &filePath)
{
    QFile file{filePath};
    if (file.open(QIODevice::ReadOnly))
        return {QString::fromUtf8(file.readAll())};

    return {};
}

QString createVersion(const QMultiHash<QString, QQmlDirParser::Component> &components)
{
    auto found = std::max_element(components.begin(), components.end(), [](auto &&first, auto &&second) {
        return first.version < second.version;
    });

    if (found != components.end() && found->version.isValid())
        return QString{"%1.%2"}.arg(found->version.majorVersion()).arg(found->version.minorVersion());

    return {};
}
#endif

constexpr auto coreModules = std::make_tuple(QStringView{u"QtQuick"},
                                             QStringView{u"QtQuick.Controls"},
                                             QStringView{u"QtQuick3D"},
                                             QStringView{u"QtQuick3D.Helpers"},
                                             QStringView{u"QtQuick3D.Particles3D"});

bool isCoreVersion(QStringView moduleName)
{
    return std::apply([=](auto... coreModuleName) { return ((moduleName == coreModuleName) || ...); },
                      coreModules);
}

QString createCoreVersion(QStringView moduleName, ExternalDependenciesInterface &externalDependencies)
{
    if (isCoreVersion(moduleName))
        return externalDependencies.qtQuickVersion();

    return {};
}

} // namespace

void ModuleScanner::scan(const QStringList &modulePaths)
{
    for (const QString &modulePath : modulePaths)
        scan(modulePath.toStdString());
}

void ModuleScanner::scan([[maybe_unused]] std::string_view modulePath)
{
#ifdef QDS_HAS_QMLPRIVATE
    QDirIterator dirIterator{QString::fromUtf8(modulePath),
                             QDir::Dirs | QDir::NoDotAndDotDot,
                             QDirIterator::Subdirectories};

    while (dirIterator.hasNext()) {
        auto directoryPath = dirIterator.next();
        QString qmldirPath = directoryPath + "/qmldir";
        if (QFileInfo::exists(qmldirPath)) {
            QQmlDirParser parser;

            auto content = contentAsQString(qmldirPath);
            if (!content)
                continue;

            bool hasError = parser.parse(*content);
            if (hasError)
                continue;

            auto moduleName = parser.typeNamespace();

            if (moduleName.isEmpty() || m_skip(moduleName))
                continue;

<<<<<<< HEAD   (0d4e8c QmlDesigner: Remove duplicates)
            m_modules.push_back(
                Import::createLibraryImport(moduleName, createVersion(parser.components())));
=======
            QString version = m_versionScanning == VersionScanning::Yes
                                  ? createVersion(parser.components())
                                  : QString{};

            QString coreModuleVersion = createCoreVersion(moduleName, m_externalDependencies);

            if (!coreModuleVersion.isEmpty())
                version = coreModuleVersion;

            m_modules.push_back(Import::createLibraryImport(moduleName, version));
>>>>>>> CHANGE (de2154 QmlDesigner: Add versioning for core modules)
        }
    }

    std::sort(m_modules.begin(), m_modules.end());
    m_modules.erase(std::unique(m_modules.begin(), m_modules.end()), m_modules.end());
#endif
}

} // namespace QmlDesigner
