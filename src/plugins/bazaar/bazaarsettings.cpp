/****************************************************************************
**
** Copyright (C) 2016 Hugues Delorme
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

#include "bazaarsettings.h"
#include "constants.h"

namespace Bazaar {
namespace Internal {

const QLatin1String BazaarSettings::diffIgnoreWhiteSpaceKey("diffIgnoreWhiteSpace");
const QLatin1String BazaarSettings::diffIgnoreBlankLinesKey("diffIgnoreBlankLines");
const QLatin1String BazaarSettings::logVerboseKey("logVerbose");
const QLatin1String BazaarSettings::logForwardKey("logForward");
const QLatin1String BazaarSettings::logIncludeMergesKey("logIncludeMerges");
const QLatin1String BazaarSettings::logFormatKey("logFormat");

BazaarSettings::BazaarSettings()
{
    setSettingsGroup(QLatin1String(Constants::BAZAAR));
    // Override default binary path
    declareKey(binaryPathKey, QLatin1String(Constants::BAZAARDEFAULT));
    declareKey(diffIgnoreWhiteSpaceKey, false);
    declareKey(diffIgnoreBlankLinesKey, false);
    declareKey(logVerboseKey, false);
    declareKey(logForwardKey, false);
    declareKey(logIncludeMergesKey, false);
    declareKey(logFormatKey, QLatin1String("long"));
}

bool BazaarSettings::sameUserId(const BazaarSettings &other) const
{
    return stringValue(userNameKey) == other.stringValue(userNameKey)
            && stringValue(userEmailKey) == other.stringValue(userEmailKey);
}

} // namespace Internal
} // namespace Bazaar
