/**************************************************************************
**
** Copyright (C) 2015 Openismus GmbH.
** Authors: Peter Penz (ppenz@openismus.com)
**          Patricia Santana Cruz (patriciasantanacruz@gmail.com)
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef AUTOTOOLSPROJECTPLUGIN_H
#define AUTOTOOLSPROJECTPLUGIN_H

#include <extensionsystem/iplugin.h>

namespace AutotoolsProjectManager {
namespace Internal {

/**
 * @brief Implementation of the ExtensionsSystem::IPlugin interface.
 *
 * The plugin creates the following components:
 *
 * - AutotoolsManager: Will manage the new autotools project and
 *   tell QtCreator for which MIME types the autotools project should
 *   be instantiated.
 *
 * - MakeStepFactory: This factory is used to create make steps.
 *
 * - AutogenStepFactory: This factory is used to create autogen steps.
 *
 * - AutoreconfStepFactory: This factory is used to create autoreconf
 *   steps.
 *
 * - ConfigureStepFactory: This factory is used to create configure steps.
 *
 * - MakefileEditorFactory: Provides a specialized editor with automatic
 *   syntax highlighting for Makefile.am files.
 *
 * - AutotoolsTargetFactory: Our current target is desktop.
 *
 * - AutotoolsBuildConfigurationFactory: Creates build configurations that
 *   contain the steps (make, autogen, autoreconf or configure) that will
 *   be executed in the build process)
 */

class AutotoolsProjectPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "AutotoolsProjectManager.json")

public:
    AutotoolsProjectPlugin();

    void extensionsInitialized();
    bool initialize(const QStringList &arguments, QString *errorString);
};

} // namespace Internal
} // namespace AutotoolsProjectManager

#endif // AUTOTOOLSPROJECTPLUGIN_H
