// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldesignercorelib_global.h>

#include <externaldependenciesinterface.h>
#include <import.h>

#include <optional>

namespace QmlDesigner {

enum class VersionScanning { No, Yes };

class QMLDESIGNERCORE_EXPORT ModuleScanner
{
public:
    using SkipFunction = std::function<bool(QStringView)>;

    ModuleScanner([[maybe_unused]] SkipFunction skip,
                  [[maybe_unused]] VersionScanning versionScanning,
                  [[maybe_unused]] ExternalDependenciesInterface &externalDependencies)
        : m_skip{std::move(skip)}
        , m_versionScanning{versionScanning}
        , m_externalDependencies{externalDependencies}
    {
        m_modules.reserve(128);
    }

    void scan(const QStringList &modulePaths);

    const Imports &modules() const { return m_modules; }

private:
    void scan(std::string_view modulePaths);

private:
    Imports m_modules;
    SkipFunction m_skip;
    VersionScanning m_versionScanning;
    ExternalDependenciesInterface &m_externalDependencies;
};

} // namespace QmlDesigner
