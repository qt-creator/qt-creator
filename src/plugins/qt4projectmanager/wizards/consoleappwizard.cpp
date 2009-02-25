/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "consoleappwizard.h"

#include "consoleappwizarddialog.h"
#include "qt4projectmanager.h"
#include "qt4projectmanagerconstants.h"

#include <utils/pathchooser.h>

#include <QtCore/QDir>
#include <QtCore/QTextStream>
#include <QtCore/QDebug>

#include <QtGui/QIcon>

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
  : QtWizard(tr("Qt4 Console Application"),
             tr("Creates a Qt4 console application."),
             QIcon(":/wizards/images/console.png"))
{
}

QWizard *ConsoleAppWizard::createWizardDialog(QWidget *parent,
                                              const QString &defaultPath,
                                              const WizardPageList &extensionPages) const
{
    ConsoleAppWizardDialog *dialog = new ConsoleAppWizardDialog(name(), icon(), extensionPages, parent);
    dialog->setPath(defaultPath.isEmpty() ? Core::Utils::PathChooser::homePath() : defaultPath);
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
    source.setContents(QLatin1String(mainCppC));
    // Create files: Profile
    const QString profileName = Core::BaseFileWizard::buildFileName(projectPath, params.name,profileSuffix());

    Core::GeneratedFile profile(profileName);
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
