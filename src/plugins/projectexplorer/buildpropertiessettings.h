/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "projectexplorer_export.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/aspects.h>

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT BuildPropertiesSettings : public Utils::AspectContainer
{
    Q_DECLARE_TR_FUNCTIONS(ProjectExplorer::Internal::BuildPropertiesSettings)

public:
    BuildPropertiesSettings();

    class BuildTriStateAspect : public Utils::TriStateAspect
    {
    public:
        BuildTriStateAspect();
    };

    Utils::StringAspect buildDirectoryTemplate;
    Utils::StringAspect buildDirectoryTemplateOld; // TODO: Remove in ~4.16
    BuildTriStateAspect separateDebugInfo;
    BuildTriStateAspect qmlDebugging;
    BuildTriStateAspect qtQuickCompiler;
    Utils::BoolAspect showQtSettings;

    void readSettings(QSettings *settings);

    QString defaultBuildDirectoryTemplate();
};

namespace Internal {

class BuildPropertiesSettingsPage final : public Core::IOptionsPage
{
public:
    explicit BuildPropertiesSettingsPage(BuildPropertiesSettings *settings);
};

} // namespace Internal
} // namespace ProjectExplorer
