/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "emptyprojectwizard.h"

#include "emptyprojectwizarddialog.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <qtsupport/qtsupportconstants.h>

#include <QCoreApplication>

namespace QmakeProjectManager {
namespace Internal {

EmptyProjectWizard::EmptyProjectWizard()
{
    setId(QLatin1String("U.Qt4Empty"));
    setCategory(QLatin1String(ProjectExplorer::Constants::QT_PROJECT_WIZARD_CATEGORY));
    setDisplayCategory(QCoreApplication::translate("ProjectExplorer",
             ProjectExplorer::Constants::QT_PROJECT_WIZARD_CATEGORY_DISPLAY));
    setDisplayName(tr("Empty Qt Project"));
    setDescription(tr("Creates a qmake-based project without any files. This allows you to create "
                "an application without any default classes."));
    setIcon(QIcon(QLatin1String(":/wizards/images/gui.png")));
    setRequiredFeatures(Core::Feature(QtSupport::Constants::FEATURE_QT));
}

QWizard *EmptyProjectWizard::create(QWidget *parent, const Core::WizardDialogParameters &parameters) const
{
    EmptyProjectWizardDialog *dialog = new EmptyProjectWizardDialog(displayName(), icon(), parent, parameters);
    dialog->setProjectName(EmptyProjectWizardDialog::uniqueProjectName(parameters.defaultPath()));
    return dialog;
}

Core::GeneratedFiles
        EmptyProjectWizard::generateFiles(const QWizard *w,
                                        QString * /*errorMessage*/) const
{
    const EmptyProjectWizardDialog *wizard = qobject_cast< const EmptyProjectWizardDialog *>(w);
    const QtProjectParameters params = wizard->parameters();
    const QString projectPath = params.projectPath();
    const QString profileName = Core::BaseFileWizardFactory::buildFileName(projectPath, params.fileName, profileSuffix());

    Core::GeneratedFile profile(profileName);
    profile.setAttributes(Core::GeneratedFile::OpenProjectAttribute | Core::GeneratedFile::OpenEditorAttribute);
    return Core::GeneratedFiles() << profile;
}

} // namespace Internal
} // namespace QmakeProjectManager
