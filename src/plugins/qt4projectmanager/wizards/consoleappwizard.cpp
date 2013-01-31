/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "consoleappwizard.h"

#include "consoleappwizarddialog.h"
#include "qt4projectmanagerconstants.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <cpptools/abstracteditorsupport.h>
#include <qtsupport/qtsupportconstants.h>

#include <QIcon>
#include <QFileInfo>
#include <QTextStream>

static const char mainCppC[] =
"#include <QCoreApplication>\n\n"
"int main(int argc, char *argv[])\n"
"{\n"
"    QCoreApplication a(argc, argv);\n\n"
"    return a.exec();\n"
"}\n";

static const char mainSourceFileC[] = "main";

namespace Qt4ProjectManager {
namespace Internal {

ConsoleAppWizard::ConsoleAppWizard()
  : QtWizard(QLatin1String("E.Qt4Core"),
             QLatin1String(ProjectExplorer::Constants::QT_APPLICATION_WIZARD_CATEGORY),
             QLatin1String(ProjectExplorer::Constants::QT_APPLICATION_WIZARD_CATEGORY_DISPLAY),
             tr("Qt Console Application"),
             tr("Creates a project containing a single main.cpp file with a stub implementation.\n\n"
                "Preselects a desktop Qt for building the application if available."),
             QIcon(QLatin1String(":/wizards/images/console.png")))
{
}

QWizard *ConsoleAppWizard::createWizardDialog(QWidget *parent,
                                              const Core::WizardDialogParameters &wizardDialogParameters) const
{
    ConsoleAppWizardDialog *dialog = new ConsoleAppWizardDialog(displayName(), icon(),
                                                                showModulesPageForApplications(), parent, wizardDialogParameters);
    dialog->setProjectName(ConsoleAppWizardDialog::uniqueProjectName(wizardDialogParameters.defaultPath()));
    return dialog;
}

Core::GeneratedFiles
        ConsoleAppWizard::generateFiles(const QWizard *w,
                                        QString * /*errorMessage*/) const
{
    const ConsoleAppWizardDialog *wizard = qobject_cast< const ConsoleAppWizardDialog *>(w);
    const QtProjectParameters params = wizard->parameters();
    const QString projectPath = params.projectPath();

    // Create files: Source

    const QString sourceFileName = Core::BaseFileWizard::buildFileName(projectPath, QLatin1String(mainSourceFileC), sourceSuffix());
    Core::GeneratedFile source(sourceFileName);
    source.setContents(CppTools::AbstractEditorSupport::licenseTemplate(sourceFileName) + QLatin1String(mainCppC));
    source.setAttributes(Core::GeneratedFile::OpenEditorAttribute);
    // Create files: Profile
    const QString profileName = Core::BaseFileWizard::buildFileName(projectPath, params.fileName, profileSuffix());

    Core::GeneratedFile profile(profileName);
    profile.setAttributes(Core::GeneratedFile::OpenProjectAttribute);
    QString contents;
    {
        QTextStream proStr(&contents);
        QtProjectParameters::writeProFileHeader(proStr);
        params.writeProFile(proStr);
        proStr << "\n\nSOURCES += " << QFileInfo(sourceFileName).fileName() << '\n';
    }
    profile.setContents(contents);
    return Core::GeneratedFiles() <<  source << profile;
}

Core::FeatureSet ConsoleAppWizard::requiredFeatures() const
{
    return Core::Feature(QtSupport::Constants::FEATURE_QT_CONSOLE);
}

} // namespace Internal
} // namespace Qt4ProjectManager
