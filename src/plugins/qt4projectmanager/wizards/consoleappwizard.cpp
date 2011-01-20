/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "consoleappwizard.h"

#include "consoleappwizarddialog.h"
#include "qt4projectmanagerconstants.h"

#include <cpptools/abstracteditorsupport.h>

#include <QtGui/QIcon>
#include <QtCore/QFileInfo>
#include <QtCore/QTextStream>

static const char *mainCppC =
"#include <QtCore/QCoreApplication>\n\n"
"int main(int argc, char *argv[])\n"
"{\n"
"    QCoreApplication a(argc, argv);\n\n"
"    return a.exec();\n"
"}\n";

static const char *mainSourceFileC = "main";

namespace Qt4ProjectManager {
namespace Internal {

ConsoleAppWizard::ConsoleAppWizard()
  : QtWizard(QLatin1String("E.Qt4Core"),
             QLatin1String(Constants::QT_APP_WIZARD_CATEGORY),
             QLatin1String(Constants::QT_APP_WIZARD_TR_SCOPE),
             QLatin1String(Constants::QT_APP_WIZARD_TR_CATEGORY),
             tr("Qt Console Application"),
             tr("Creates a project containing a single main.cpp file with a stub implementation.\n\n"
                "Preselects a desktop Qt for building the application if available."),
             QIcon(QLatin1String(":/wizards/images/console.png")))
{
}

QWizard *ConsoleAppWizard::createWizardDialog(QWidget *parent,
                                              const QString &defaultPath,
                                              const WizardPageList &extensionPages) const
{
    ConsoleAppWizardDialog *dialog = new ConsoleAppWizardDialog(displayName(), icon(), extensionPages,
                                                                showModulesPageForApplications(), parent);
    dialog->setPath(defaultPath);
    dialog->setProjectName(ConsoleAppWizardDialog::uniqueProjectName(defaultPath));
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

} // namespace Internal
} // namespace Qt4ProjectManager
