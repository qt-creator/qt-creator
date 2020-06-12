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

#include "pythonplugin.h"

#include "pythoneditor.h"
#include "pythonproject.h"
#include "pythonsettings.h"
#include "pythonrunconfiguration.h"
#include "pythonutils.h"

#include <coreplugin/fileiconprovider.h>

#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/localenvironmentaspect.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/taskhub.h>

#include <utils/theme/theme.h>

using namespace ProjectExplorer;

namespace Python {
namespace Internal {

////////////////////////////////////////////////////////////////////////////////////
//
// PythonPlugin
//
////////////////////////////////////////////////////////////////////////////////////

static PythonPlugin *m_instance = nullptr;

class PythonPluginPrivate
{
public:
    PythonEditorFactory editorFactory;
    PythonOutputFormatterFactory outputFormatterFactory;
    PythonRunConfigurationFactory runConfigFactory;

    RunWorkerFactory runWorkerFactory{
        RunWorkerFactory::make<SimpleTargetRunner>(),
        {ProjectExplorer::Constants::NORMAL_RUN_MODE},
        {runConfigFactory.runConfigurationId()}
    };
};

PythonPlugin::PythonPlugin()
{
    m_instance = this;
}

PythonPlugin::~PythonPlugin()
{
    m_instance = nullptr;
    delete d;
}

PythonPlugin *PythonPlugin::instance()
{
    return m_instance;
}

bool PythonPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    d = new PythonPluginPrivate;

    ProjectManager::registerProjectType<PythonProject>(PythonMimeType);

    PythonSettings::init();

    return true;
}

void PythonPlugin::extensionsInitialized()
{
    // Add MIME overlay icons (these icons displayed at Project dock panel)
    QString imageFile = Utils::creatorTheme()->imageFile(Utils::Theme::IconOverlayPro,
                                                         Constants::FILEOVERLAY_PY);
    Core::FileIconProvider::registerIconOverlayForSuffix(imageFile, "py");

    TaskHub::addCategory(PythonErrorTaskCategory, "Python", true);

    connect(Core::EditorManager::instance(), &Core::EditorManager::documentOpened,
            this, &PyLSConfigureAssistant::documentOpened);
}

} // namespace Internal
} // namespace Python
