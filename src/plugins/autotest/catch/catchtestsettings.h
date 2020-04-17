/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#pragma once

#include "../iframeworksettings.h"

namespace Autotest {
namespace Internal {

class CatchTestSettings : public IFrameworkSettings
{
public:
    CatchTestSettings() = default;
    QString name() const override;

    int abortAfter = 0;
    int benchmarkSamples = 0;
    int benchmarkResamples = 0;
    double confidenceInterval = 0;
    int benchmarkWarmupTime = 0;
    bool abortAfterChecked = false;
    bool samplesChecked = false;
    bool resamplesChecked = false;
    bool confidenceIntervalChecked = false;
    bool warmupChecked = false;
    bool noAnalysis = false;
    bool showSuccess = false;
    bool breakOnFailure = true;
    bool noThrow = false;
    bool visibleWhitespace = false;
    bool warnOnEmpty = false;

protected:
    void toFrameworkSettings(QSettings *s) const override;
    void fromFrameworkSettings(const QSettings *s) override;
};

} // namespace Internal
} // namespace Autotest
