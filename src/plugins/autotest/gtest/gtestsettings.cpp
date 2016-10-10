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

void GTestSettings::fromFrameworkSettings(const QSettings *s)
{
    runDisabled = s->value(runDisabledKey, false).toBool();
    repeat = s->value(repeatKey, false).toBool();
    shuffle = s->value(shuffleKey, false).toBool();
    iterations = s->value(iterationsKey, 1).toInt();
    seed = s->value(seedKey, 0).toInt();
    breakOnFailure = s->value(breakOnFailureKey, true).toBool();
    throwOnFailure = s->value(throwOnFailureKey, false).toBool();
}

void GTestSettings::toFrameworkSettings(QSettings *s) const
{
    s->setValue(runDisabledKey, runDisabled);
    s->setValue(repeatKey, repeat);
    s->setValue(shuffleKey, shuffle);
    s->setValue(iterationsKey, iterations);
    s->setValue(seedKey, seed);
    s->setValue(breakOnFailureKey, breakOnFailure);
    s->setValue(throwOnFailureKey, throwOnFailure);
}

} // namespace Internal
} // namespace Autotest
