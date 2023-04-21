// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "modelutils.h"

#include <utils/expected.h>

#include <algorithm>

namespace QmlDesigner::Utils {

namespace {

enum class ImportError { EmptyImportName, HasAlreadyImport, NoModule };

::Utils::expected<Import, ImportError> findImport(const QString &importName,
                                                  const std::function<bool(const Import &)> &predicate,
                                                  const Imports &imports,
                                                  const Imports &modules)
{
    if (importName.isEmpty())
        return ::Utils::make_unexpected(ImportError::EmptyImportName);

    auto hasName = [&](const auto &import) {
        return import.url() == importName || import.file() == importName;
    };

    bool hasImport = std::any_of(imports.begin(), imports.end(), hasName);

    if (hasImport)
        return ::Utils::make_unexpected(ImportError::HasAlreadyImport);

    auto foundModule = std::find_if(modules.begin(), modules.end(), [&](const Import &import) {
        return hasName(import) && predicate(import);
    });

    if (foundModule == modules.end())
        return ::Utils::make_unexpected(ImportError::NoModule);

    return *foundModule;
}

} // namespace

bool addImportWithCheck(const QString &importName,
                        const std::function<bool(const Import &)> &predicate,
                        Model *model)
{
    return addImportsWithCheck({importName}, predicate, model);
}

bool addImportWithCheck(const QString &importName, Model *model)
{
    return addImportWithCheck(
        importName, [](const Import &) { return true; }, model);
}

bool addImportsWithCheck(const QStringList &importNames, Model *model)
{
    return addImportsWithCheck(
        importNames, [](const Import &) { return true; }, model);
}

bool addImportsWithCheck(const QStringList &importNames,
                         const std::function<bool(const Import &)> &predicate,
                         Model *model)
{
    const Imports &imports = model->imports();
    const Imports &modules = model->possibleImports();

    Imports importsToAdd;
    importsToAdd.reserve(importNames.size());

    for (const QString &importName : importNames) {
        auto import = findImport(importName, predicate, imports, modules);

        if (import) {
            importsToAdd.push_back(*import);
        } else {
            if (import.error() == ImportError::NoModule)
                return false;
            else
                continue;
        }
    }

    if (!importsToAdd.isEmpty())
        model->changeImports(std::move(importsToAdd), {});

    return true;
}

} // namespace QmlDesigner::Utils
