// Copyright (C) 2020 Uwe Kindler
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1-or-later OR GPL-3.0-or-later

#pragma once

#include "ads_globals.h"

#include <QIcon>

namespace ADS {

struct IconProviderPrivate;

/**
 * This object provides all icons that are required by the advanced docking system.
 * The IconProvider enables the user to register custom icons in case using
 * stylesheets is not an option.
 */
class ADS_EXPORT IconProvider
{
private:
    IconProviderPrivate *d; ///< private data (pimpl)
    friend struct IconProviderPrivate;

public:
    /**
     * Default Constructor
     */
    IconProvider();

    /**
     * Virtual Destructor
     */
    virtual ~IconProvider();

    /**
     * The function returns a custom icon if one is registered and a null Icon
     * if no custom icon is registered
     */
    QIcon customIcon(eIcon iconId) const;

    /**
     * Registers a custom icon for the given IconId
     */
    void registerCustomIcon(eIcon iconId, const QIcon &icon);
}; // class IconProvider

} // namespace ADS
