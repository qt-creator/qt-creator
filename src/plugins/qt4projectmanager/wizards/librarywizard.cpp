/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "librarywizard.h"
#include "librarywizarddialog.h"
#include "qt4projectmanager.h"
#include "qt4projectmanagerconstants.h"

#include <utils/codegeneration.h>
#include <cpptools/cppmodelmanagerinterface.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QTextStream>
#include <QtGui/QIcon>

static const char *sharedHeaderPostfixC = "_global";

namespace Qt4ProjectManager {

namespace Internal {

LibraryWizard::LibraryWizard()
  : QtWizard(QLatin1String("H.Qt4Library"),
             QLatin1String(ProjectExplorer::Constants::PROJECT_WIZARD_CATEGORY),
             QLatin1String(ProjectExplorer::Constants::PROJECT_WIZARD_TR_SCOPE),
             QLatin1String(ProjectExplorer::Constants::PROJECT_WIZARD_TR_CATEGORY),
             tr("C++ Library"),
             tr("Creates a Qt based C++ Library."),
             QIcon(QLatin1String(":/wizards/images/lib.png")))
{
}

QWizard *LibraryWizard::createWizardDialog(QWidget *parent,
                                          const QString &defaultPath,
                                          const WizardPageList &extensionPages) const
{
    LibraryWizardDialog *dialog = new  LibraryWizardDialog(displayName(), icon(), extensionPages,
                                                           showModulesPageForLibraries(), parent);
    dialog->setLowerCaseFiles(QtWizard::lowerCaseFiles());
    dialog->setPath(defaultPath);
    dialog->setProjectName(LibraryWizardDialog::uniqueProjectName(defaultPath));
    dialog->setSuffixes(headerSuffix(), sourceSuffix(), formSuffix());
    return dialog;
}


Core::GeneratedFiles LibraryWizard::generateFiles(const QWizard *w,
                                                 QString *errorMessage) const
{
    Q_UNUSED(errorMessage)
    const LibraryWizardDialog *dialog = qobject_cast<const LibraryWizardDialog *>(w);
    const QtProjectParameters projectParams = dialog->parameters();
    const QString projectPath = projectParams.projectPath();
    const LibraryParameters params = dialog->libraryParameters();
    const QString license = CppTools::AbstractEditorSupport::licenseTemplate();

    const QString sharedLibExportMacro = QtProjectParameters::exportMacro(projectParams.fileName);

    Core::GeneratedFiles rc;
    // Class header + source
    const QString sourceFileName = buildFileName(projectPath, params.sourceFileName, sourceSuffix());
    Core::GeneratedFile source(sourceFileName);

    const QString headerFileFullName = buildFileName(projectPath, params.headerFileName, headerSuffix());
    const QString headerFileName = QFileInfo(headerFileFullName).fileName();
    Core::GeneratedFile header(headerFileFullName);

    // Create files: global header for shared libs
    QString globalHeaderFileName;
    if (projectParams.type == QtProjectParameters::SharedLibrary) {
        const QString globalHeaderName = buildFileName(projectPath, projectParams.fileName + QLatin1String(sharedHeaderPostfixC), headerSuffix());
        Core::GeneratedFile globalHeader(globalHeaderName);
        globalHeaderFileName = QFileInfo(globalHeader.path()).fileName();
        globalHeader.setContents(license + LibraryParameters::generateSharedHeader(globalHeaderFileName, projectParams.fileName, sharedLibExportMacro));
        rc.push_back(globalHeader);
    }

    // Generate code
    QString headerContents, sourceContents;
    params.generateCode(projectParams.type, projectParams.fileName,  headerFileName,
                        globalHeaderFileName, sharedLibExportMacro,
                        /* indentation*/ 4, &headerContents, &sourceContents);

    source.setContents(license + sourceContents);
    header.setContents(license + headerContents);
    rc.push_back(source);
    rc.push_back(header);
    // Create files: profile
    const QString profileName = buildFileName(projectPath, projectParams.fileName, profileSuffix());
    Core::GeneratedFile profile(profileName);
    QString profileContents;
    {
        QTextStream proStr(&profileContents);
        QtProjectParameters::writeProFileHeader(proStr);
        projectParams.writeProFile(proStr);
        proStr << "\nSOURCES += " << QFileInfo(source.path()).fileName()
               << "\n\nHEADERS += " << headerFileName;
        if (!globalHeaderFileName.isEmpty())
            proStr << "\\\n        " << globalHeaderFileName << '\n';
    }
    profile.setContents(profileContents);
    rc.push_back(profile);
    return rc;
}

} // namespace Internal
} // namespace Qt4ProjectManager
