/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "gtestsettings.h"

namespace Autotest {
namespace Internal {

static const char breakOnFailureKey[]   = "BreakOnFailure";
static const char iterationsKey[]       = "Iterations";
static const char repeatKey[]           = "Repeat";
static const char runDisabledKey[]      = "RunDisabled";
static const char seedKey[]             = "Seed";
static const char shuffleKey[]          = "Shuffle";
static const char throwOnFailureKey[]   = "ThrowOnFailure";

QString GTestSettings::name() const
{
    return QString("GTest");
}

void GTestSettings::fromSettings(const QSettings *s)
{
    runDisabled = s->value(QLatin1String(runDisabledKey), false).toBool();
    repeat = s->value(QLatin1String(repeatKey), false).toBool();
    shuffle = s->value(QLatin1String(shuffleKey), false).toBool();
    iterations = s->value(QLatin1String(iterationsKey), 1).toInt();
    seed = s->value(QLatin1String(seedKey), 0).toInt();
    breakOnFailure = s->value(QLatin1String(breakOnFailureKey), true).toBool();
    throwOnFailure = s->value(QLatin1String(throwOnFailureKey), false).toBool();
}

void GTestSettings::toSettings(QSettings *s) const
{
    s->setValue(QLatin1String(runDisabledKey), runDisabled);
    s->setValue(QLatin1String(repeatKey), repeat);
    s->setValue(QLatin1String(shuffleKey), shuffle);
    s->setValue(QLatin1String(iterationsKey), iterations);
    s->setValue(QLatin1String(seedKey), seed);
    s->setValue(QLatin1String(breakOnFailureKey), breakOnFailure);
    s->setValue(QLatin1String(throwOnFailureKey), throwOnFailure);
}

} // namespace Internal
} // namespace Autotest
