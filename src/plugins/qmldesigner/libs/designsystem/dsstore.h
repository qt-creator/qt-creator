// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <dsthememanager.h>
#include <externaldependenciesinterface.h>

namespace QmlDesigner {
class DSThemeManager;
class ExternalDependenciesInterface;

using DSCollections = std::map<QString, DSThemeManager>;

class DSStore
{
    Q_DECLARE_TR_FUNCTIONS(DSStore);

public:
    DSStore(ExternalDependenciesInterface &ed);
    ~DSStore();

    QString moduleImportStr() const;

    std::optional<QString> load();
    std::optional<QString> load(const Utils::FilePath &dsModuleDirPath);

    std::optional<QString> save(bool mcuCompatible = false) const;
    std::optional<QString> save(const Utils::FilePath &moduleDirPath, bool mcuCompatible = false) const;

    size_t collectionCount() const { return m_collections.size(); }

    DSThemeManager *addCollection(const QString &qmlTypeName);
    std::optional<QString> typeName(DSThemeManager *collection) const;

private:
    std::optional<QString> loadCollection(const QString &typeName, const Utils::FilePath &qmlFilePath);
    std::optional<QString> writeQml(const DSThemeManager &mgr,
                                    const QString &typeName,
                                    const Utils::FilePath &targetDir,
                                    bool mcuCompatible) const;

private:
    ExternalDependenciesInterface &m_ed;
    DSCollections m_collections;
    std::map<DSThemeManager *, const QString &> m_collectionTypeNames;
};
} // namespace QmlDesigner
