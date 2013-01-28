/**************************************************************************
**
** Copyright (C) 2013 Openismus GmbH.
** Authors: Peter Penz (ppenz@openismus.com)
**          Patricia Santana Cruz (patriciasantanacruz@gmail.com)
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "autotoolsmanager.h"
#include "autotoolsproject.h"
#include "autotoolsprojectconstants.h"

#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/messagemanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>

using namespace ProjectExplorer;

namespace AutotoolsProjectManager {
namespace Internal {

Project *AutotoolsManager::openProject(const QString &fileName, QString *errorString)
{
    QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();

    if (canonicalFilePath.isEmpty()) {
        if (errorString)
            *errorString = tr("Failed opening project '%1': Project file does not exist")
                             .arg(QDir::toNativeSeparators(fileName));
        return 0;
    }

    // Check whether the project is already open or not.
    ProjectExplorerPlugin *projectExplorer = ProjectExplorerPlugin::instance();
    foreach (Project *pi, projectExplorer->session()->projects()) {
        if (canonicalFilePath == pi->document()->fileName()) {
            *errorString = tr("Failed opening project '%1': Project already open")
                             .arg(QDir::toNativeSeparators(canonicalFilePath));
            return 0;
        }
    }

    return new AutotoolsProject(this, canonicalFilePath);
}

QString AutotoolsManager::mimeType() const
{
    return QLatin1String(Constants::MAKEFILE_MIMETYPE);
}

} // namespace Internal
} // namespace AutotoolsProjectManager
