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

#include "librarywizard.h"
#include "librarywizarddialog.h"
#include "mobilelibraryparameters.h"

#include <cpptools/abstracteditorsupport.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <qtsupport/qtsupportconstants.h>

#include <QFileInfo>
#include <QTextStream>
#include <QIcon>

static const char sharedHeaderPostfixC[] = "_global";

namespace Qt4ProjectManager {

namespace Internal {

LibraryWizard::LibraryWizard()
  : QtWizard(QLatin1String("H.Qt4Library"),
             QLatin1String(ProjectExplorer::Constants::LIBRARIES_WIZARD_CATEGORY),
             QLatin1String(ProjectExplorer::Constants::LIBRARIES_WIZARD_CATEGORY_DISPLAY),
             tr("C++ Library"),
             tr("Creates a C++ library based on qmake. This can be used to create:<ul>"
                "<li>a shared C++ library for use with <tt>QPluginLoader</tt> and runtime (Plugins)</li>"
                "<li>a shared or static C++ library for use with another project at linktime</li></ul>"),
             QIcon(QLatin1String(":/wizards/images/lib.png")))
{
}

QWizard *LibraryWizard::createWizardDialog(QWidget *parent, const Core::WizardDialogParameters &wizardDialogParameters) const
{
    LibraryWizardDialog *dialog = new LibraryWizardDialog(displayName(),
                                                           icon(),
                                                           showModulesPageForLibraries(),
                                                           parent,
                                                           wizardDialogParameters);
    dialog->setLowerCaseFiles(QtWizard::lowerCaseFiles());
    dialog->setProjectName(LibraryWizardDialog::uniqueProjectName(wizardDialogParameters.defaultPath()));
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
    QString pluginJsonFileFullName;
    QString pluginJsonFileName;
    if (projectParams.type == QtProjectParameters::Qt4Plugin) {
        pluginJsonFileFullName = buildFileName(projectPath, projectParams.fileName, QLatin1String("json"));
        pluginJsonFileName = QFileInfo(pluginJsonFileFullName).fileName();
    }

    Core::GeneratedFile header(headerFileFullName);

    // Create files: global header for shared libs
    QString globalHeaderFileName;
    if (projectParams.type == QtProjectParameters::SharedLibrary) {
        const QString globalHeaderName = buildFileName(projectPath, projectParams.fileName.toLower() + QLatin1String(sharedHeaderPostfixC), headerSuffix());
        Core::GeneratedFile globalHeader(globalHeaderName);
        globalHeaderFileName = QFileInfo(globalHeader.path()).fileName();
        globalHeader.setContents(CppTools::AbstractEditorSupport::licenseTemplate(globalHeaderFileName)
                                 + LibraryParameters::generateSharedHeader(globalHeaderFileName, projectParams.fileName, sharedLibExportMacro));
        rc.push_back(globalHeader);
    }

    // Generate code
    QString headerContents, sourceContents;
    params.generateCode(projectParams.type, projectParams.fileName,  headerFileName,
                        globalHeaderFileName, sharedLibExportMacro, pluginJsonFileName,
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
        if (!pluginJsonFileName.isEmpty())
            proStr << "\nOTHER_FILES += " << pluginJsonFileName << '\n';
        if (mobileParams.type)
            mobileParams.writeProFile(proStr);
    }
    profile.setContents(profileContents);
    rc.push_back(profile);

    if (!pluginJsonFileName.isEmpty()) {
        Core::GeneratedFile jsonFile(pluginJsonFileFullName);
        jsonFile.setContents(QLatin1String("{\n    \"Keys\" : [ ]\n}\n"));
        rc.push_back(jsonFile);
    }
    return rc;
}

Core::FeatureSet LibraryWizard::requiredFeatures() const
{
    return Core::Feature(QtSupport::Constants::FEATURE_QT);
}

} // namespace Internal
} // namespace Qt4ProjectManager
