/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010-2011 Openismus GmbH.
**   Authors: Peter Penz (ppenz@openismus.com)
**            Patricia Santana Cruz (patriciasantanacruz@gmail.com)
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef AUTOTOOLSPROJECTMANAGER_H
#define AUTOTOOLSPROJECTMANAGER_H

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
public:
    AutotoolsProjectPlugin();

    void extensionsInitialized();
    bool initialize(const QStringList &arguments, QString *errorString);
};

} // namespace Internal
} // namespace AutotoolsProjectManager

#endif // AUTOTOOLSPROJECTMANAGER_H
