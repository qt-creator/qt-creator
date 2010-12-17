/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "librarywizard.h"
#include "librarywizarddialog.h"
#include "qt4projectmanager.h"
#include "qt4projectmanagerconstants.h"
#include "mobilelibraryparameters.h"

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
             tr("Creates a C++ library based on qmake. This can be used to create:<ul>"
                "<li>a shared C++ library for use with <tt>QPluginLoader</tt> and runtime (Plugins)</li>"
                "<li>a shared or static C++ library for use with another project at linktime</li></ul>."),
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
    const MobileLibraryParameters mobileParams = dialog->mobileLibraryParameters();

    const QString sharedLibExportMacro = QtProjectParameters::exportMacro(projectParams.fileName);

    Core::GeneratedFiles rc;
    // Class header + source
    const QString sourceFileName = buildFileName(projectPath, params.sourceFileName, sourceSuffix());
    Core::GeneratedFile source(sourceFileName);
    source.setAttributes(Core::GeneratedFile::OpenEditorAttribute);

    const QString headerFileFullName = buildFileName(projectPath, params.headerFileName, headerSuffix());
    const QString headerFileName = QFileInfo(headerFileFullName).fileName();
    Core::GeneratedFile header(headerFileFullName);

    // Create files: global header for shared libs
    QString globalHeaderFileName;
    if (projectParams.type == QtProjectParameters::SharedLibrary) {
        const QString globalHeaderName = buildFileName(projectPath, projectParams.fileName + QLatin1String(sharedHeaderPostfixC), headerSuffix());
        Core::GeneratedFile globalHeader(globalHeaderName);
        globalHeaderFileName = QFileInfo(globalHeader.path()).fileName();
        globalHeader.setContents(CppTools::AbstractEditorSupport::licenseTemplate(globalHeaderFileName)
                                 + LibraryParameters::generateSharedHeader(globalHeaderFileName, projectParams.fileName, sharedLibExportMacro));
        rc.push_back(globalHeader);
    }

    // Generate code
    QString headerContents, sourceContents;
    params.generateCode(projectParams.type, projectParams.fileName,  headerFileName,
                        globalHeaderFileName, sharedLibExportMacro,
                        /* indentation*/ 4, &headerContents, &sourceContents);

    source.setContents(CppTools::AbstractEditorSupport::licenseTemplate(sourceFileName, params.className)
                       + sourceContents);
    header.setContents(CppTools::AbstractEditorSupport::licenseTemplate(headerFileFullName, params.className)
                       + headerContents);
    rc.push_back(source);
    rc.push_back(header);
    // Create files: profile
    const QString profileName = buildFileName(projectPath, projectParams.fileName, profileSuffix());
    Core::GeneratedFile profile(profileName);
    profile.setAttributes(Core::GeneratedFile::OpenProjectAttribute);
    QString profileContents;
    {
        QTextStream proStr(&profileContents);
        QtProjectParameters::writeProFileHeader(proStr);
        projectParams.writeProFile(proStr);
        proStr << "\nSOURCES += " << QFileInfo(source.path()).fileName()
               << "\n\nHEADERS += " << headerFileName;
        if (!globalHeaderFileName.isEmpty())
            proStr << "\\\n        " << globalHeaderFileName << '\n';
        if (mobileParams.type)
            mobileParams.writeProFile(proStr);
    }
    profile.setContents(profileContents);
    rc.push_back(profile);
    return rc;
}

} // namespace Internal
} // namespace Qt4ProjectManager
