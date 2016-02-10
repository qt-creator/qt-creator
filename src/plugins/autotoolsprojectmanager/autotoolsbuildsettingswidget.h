/****************************************************************************
**
** Copyright (C) 2016 Openismus GmbH.
** Author: Peter Penz (ppenz@openismus.com)
** Author: Patricia Santana Cruz (patriciasantanacruz@gmail.com)
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

#include <projectexplorer/namedwidget.h>

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

namespace Utils { class PathChooser; }

namespace AutotoolsProjectManager {
namespace Internal {

class AutotoolsBuildConfiguration;

/**
 * @brief Implementation of ProjectExplorer::BuildConfigWidget interface.
 *
 * Provides an editor to configure the build directory and build steps.
 */
class AutotoolsBuildSettingsWidget : public ProjectExplorer::NamedWidget
{
    Q_OBJECT

public:
    AutotoolsBuildSettingsWidget(AutotoolsBuildConfiguration *bc);

private:
    void buildDirectoryChanged();
    void environmentHasChanged();

    Utils::PathChooser *m_pathChooser;
    AutotoolsBuildConfiguration *m_buildConfiguration;
};

} // namespace Internal
} // namespace AutotoolsProjectManager
