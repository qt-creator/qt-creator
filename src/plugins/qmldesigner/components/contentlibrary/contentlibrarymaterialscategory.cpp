// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "contentlibrarymaterialscategory.h"

#include "contentlibrarymaterial.h"

namespace QmlDesigner {

ContentLibraryMaterialsCategory::ContentLibraryMaterialsCategory(QObject *parent, const QString &name)
    : QObject(parent), m_name(name) {}

void ContentLibraryMaterialsCategory::addBundleMaterial(ContentLibraryMaterial *bundleMat)
{
    m_categoryMaterials.append(bundleMat);
}

bool ContentLibraryMaterialsCategory::updateImportedState(const QStringList &importedMats)
{
    bool changed = false;

    for (ContentLibraryMaterial *mat : std::as_const(m_categoryMaterials))
        changed |= mat->setImported(importedMats.contains(mat->qml().chopped(4)));

    return changed;
}

bool ContentLibraryMaterialsCategory::filter(const QString &searchText)
{
    bool visible = false;
    for (ContentLibraryMaterial *mat : std::as_const(m_categoryMaterials))
        visible |= mat->filter(searchText);

    if (visible != m_visible) {
        m_visible = visible;
        emit categoryVisibleChanged();
        return true;
    }

    return false;
}

QString ContentLibraryMaterialsCategory::name() const
{
    return m_name;
}

bool ContentLibraryMaterialsCategory::visible() const
{
    return m_visible;
}

bool ContentLibraryMaterialsCategory::expanded() const
{
    return m_expanded;
}

QList<ContentLibraryMaterial *> ContentLibraryMaterialsCategory::categoryMaterials() const
{
    return m_categoryMaterials;
}

} // namespace QmlDesigner
