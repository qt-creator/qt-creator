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

#include "qmlnewprojectwizard.h"

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QtCore/QTextStream>
#include <QtCore/QCoreApplication>

using namespace QmlProjectManager::Internal;

//////////////////////////////////////////////////////////////////////////////
// QmlNewProjectWizardDialog
//////////////////////////////////////////////////////////////////////////////

QmlNewProjectWizardDialog::QmlNewProjectWizardDialog(QWidget *parent) :
    ProjectExplorer::BaseProjectWizardDialog(parent)
{
    setWindowTitle(tr("New QML Project"));
    setIntroDescription(tr("This wizard generates a QML application project."));
}

QmlNewProjectWizard::QmlNewProjectWizard()
    : Core::BaseFileWizard(parameters())
{ }

QmlNewProjectWizard::~QmlNewProjectWizard()
{ }

Core::BaseFileWizardParameters QmlNewProjectWizard::parameters()
{
    static Core::BaseFileWizardParameters parameters(ProjectWizard);
    parameters.setIcon(QIcon(QLatin1String(":/wizards/images/console.png")));
    parameters.setDisplayName(tr("QML Application"));
    parameters.setId(QLatin1String("QA.QML Application"));
    parameters.setDescription(tr("Creates a QML application."));
    parameters.setCategory(QLatin1String(ProjectExplorer::Constants::PROJECT_WIZARD_CATEGORY));
    parameters.setDisplayCategory(QCoreApplication::translate("ProjectExplorer", ProjectExplorer::Constants::PROJECT_WIZARD_TR_CATEGORY));
    return parameters;
}

QWizard *QmlNewProjectWizard::createWizardDialog(QWidget *parent,
                                                  const QString &defaultPath,
                                                  const WizardPageList &extensionPages) const
{
    QmlNewProjectWizardDialog *wizard = new QmlNewProjectWizardDialog(parent);

    wizard->setPath(defaultPath);
    wizard->setProjectName(QmlNewProjectWizardDialog::uniqueProjectName(defaultPath));

    foreach (QWizardPage *p, extensionPages)
        wizard->addPage(p);

    return wizard;
}

Core::GeneratedFiles QmlNewProjectWizard::generateFiles(const QWizard *w,
                                                     QString *errorMessage) const
{
    Q_UNUSED(errorMessage)

    const QmlNewProjectWizardDialog *wizard = qobject_cast<const QmlNewProjectWizardDialog *>(w);
    const QString projectName = wizard->projectName();
    const QString projectPath = wizard->path() + QLatin1Char('/') + projectName;

    const QString creatorFileName = Core::BaseFileWizard::buildFileName(projectPath,
                                                                        projectName,
                                                                        QLatin1String("qmlproject"));

    const QString mainFileName = Core::BaseFileWizard::buildFileName(projectPath,
                                                                     projectName,
                                                                     QLatin1String("qml"));

    QString contents;
    QTextStream out(&contents);

    out
        << "import Qt 4.6" << endl
        << endl
        << "Rectangle {" << endl
        << "    width: 200" << endl
        << "    height: 200" << endl
        << "    Text {" << endl
        << "        text: \"Hello World\"" << endl
        << "        anchors.centerIn: parent" << endl
        << "    }" << endl
        << "}" << endl;

    Core::GeneratedFile generatedMainFile(mainFileName);
    generatedMainFile.setContents(contents);

    Core::GeneratedFile generatedCreatorFile(creatorFileName);
    generatedCreatorFile.setContents(projectName + QLatin1String(".qml\n"));

    Core::GeneratedFiles files;
    files.append(generatedMainFile);
    files.append(generatedCreatorFile);

    return files;
}

bool QmlNewProjectWizard::postGenerateFiles(const Core::GeneratedFiles &l, QString *errorMessage)
{
    // Post-Generate: Open the project
    const QString proFileName = l.back().path();
    if (!ProjectExplorer::ProjectExplorerPlugin::instance()->openProject(proFileName)) {
        *errorMessage = tr("The project %1 could not be opened.").arg(proFileName);
        return false;
    }
    return true;
}

