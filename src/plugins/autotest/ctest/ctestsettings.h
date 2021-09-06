/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/aspects.h>

namespace Autotest {
namespace Internal {

class CTestSettings : public Utils::AspectContainer
{
    Q_DECLARE_TR_FUNCTIONS(Autotest::Internal::CTestSettings)
public:
    CTestSettings();

    QStringList activeSettingsAsOptions() const;

    Utils::IntegerAspect repetitionCount;
    Utils::SelectionAspect repetitionMode;
    Utils::SelectionAspect outputMode;
    Utils::BoolAspect outputOnFail;
    Utils::BoolAspect stopOnFailure;
    Utils::BoolAspect scheduleRandom;
    Utils::BoolAspect repeat;
    // FIXME.. this makes the outputreader fail to get all results correctly for visual display
    Utils::BoolAspect parallel;
    Utils::IntegerAspect jobs;
    Utils::BoolAspect testLoad;
    Utils::IntegerAspect threshold;
};

class CTestSettingsPage final : public Core::IOptionsPage
{
    Q_DECLARE_TR_FUNCTIONS(Autotest::Internal::CTestSettingsPage)
public:
    CTestSettingsPage(CTestSettings *settings, Utils::Id settingsId);
};

} // namespace Internal
} // namespace Autotest

