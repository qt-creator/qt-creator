// Copyright (C) 2020 Uwe Kindler
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1-or-later OR GPL-3.0-or-later

#include "iconprovider.h"

#include <QVector>

namespace ADS {

/**
 * Private data class (pimpl)
 */
struct IconProviderPrivate
{
    IconProvider *q;
    QVector<QIcon> m_userIcons{IconCount, QIcon()};

    /**
     * Private data constructor
     */
    IconProviderPrivate(IconProvider *parent);
};
// struct IconProviderPrivate

IconProviderPrivate::IconProviderPrivate(IconProvider *parent)
    : q(parent)
{}

IconProvider::IconProvider()
    : d(new IconProviderPrivate(this))
{}

IconProvider::~IconProvider()
{
    delete d;
}

QIcon IconProvider::customIcon(eIcon iconId) const
{
    Q_ASSERT(iconId < d->m_userIcons.size());
    return d->m_userIcons[iconId];
}

void IconProvider::registerCustomIcon(eIcon iconId, const QIcon &icon)
{
    Q_ASSERT(iconId < d->m_userIcons.size());
    d->m_userIcons[iconId] = icon;
}

} // namespace ADS
