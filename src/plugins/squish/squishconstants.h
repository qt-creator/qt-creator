/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator Squish plugin.
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
#ifndef SQUISHCONSTANTS_H
#define SQUISHCONSTANTS_H

#include <QtGlobal>

namespace Squish {
namespace Constants {

const char SQUISH_ID[]                    = "SquishPlugin.Squish";
const char SQUISH_CONTEXT[]               = "Squish";
const char SQUISH_SETTINGS_CATEGORY[]     = "ZYY.Squish";

// MIME type defined by Squish plugin
const char SQUISH_OBJECTSMAP_MIMETYPE[] = "text/squish-objectsmap";
const char OBJECTSMAP_EDITOR_ID[]       = "Squish.ObjectsMapEditor";

} // namespace Constants
} // namespace Squish

#endif // SQUISHCONSTANTS_H
