/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "bundlematerialcategory.h"

#include "bundlematerial.h"

namespace QmlDesigner {

BundleMaterialCategory::BundleMaterialCategory(QObject *parent, const QString &name)
    : QObject(parent), m_name(name) {}

void BundleMaterialCategory::addBundleMaterial(BundleMaterial *bundleMat)
{
    m_categoryMaterials.append(bundleMat);
}

bool BundleMaterialCategory::updateImportedState(const QStringList &importedMats)
{
    bool changed = false;

    for (BundleMaterial *mat : std::as_const(m_categoryMaterials))
        changed |= mat->setImported(importedMats.contains(mat->qml().chopped(4)));

    return changed;
}

bool BundleMaterialCategory::filter(const QString &searchText)
{
    bool visible = false;
    for (BundleMaterial *mat : std::as_const(m_categoryMaterials))
        visible |= mat->filter(searchText);

    if (visible != m_visible) {
        m_visible = visible;
        emit categoryVisibleChanged();
        return true;
    }

    return false;
}

QString BundleMaterialCategory::name() const
{
    return m_name;
}

bool BundleMaterialCategory::visible() const
{
    return m_visible;
}

QList<BundleMaterial *> BundleMaterialCategory::categoryMaterials() const
{
    return m_categoryMaterials;
}

} // namespace QmlDesigner
