/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qmlprojectapplicationwizard.h"

#include "qmlprojectconstants.h"

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QtGui/QIcon>

#include <QtCore/QTextStream>
#include <QtCore/QCoreApplication>

namespace QmlProjectManager {
namespace Internal {

QmlProjectApplicationWizardDialog::QmlProjectApplicationWizardDialog(QWidget *parent) :
    ProjectExplorer::BaseProjectWizardDialog(parent)
{
    setWindowTitle(tr("New QML Project"));
    setIntroDescription(tr("This wizard generates a QML application project."));
}

QmlProjectApplicationWizard::QmlProjectApplicationWizard()
    : Core::BaseFileWizard(parameters())
{ }

QmlProjectApplicationWizard::~QmlProjectApplicationWizard()
{ }

Core::BaseFileWizardParameters QmlProjectApplicationWizard::parameters()
{
    Core::BaseFileWizardParameters parameters(ProjectWizard);
    parameters.setIcon(QIcon(QLatin1String(":/wizards/images/console.png")));
    parameters.setDisplayName(tr("Qt QML Application"));
    parameters.setId(QLatin1String("QA.QML Application"));
    parameters.setDescription(tr("Creates a Qt QML application."));
    parameters.setCategory(QLatin1String(Constants::QML_WIZARD_CATEGORY));
    parameters.setDisplayCategory(QCoreApplication::translate(Constants::QML_WIZARD_TR_SCOPE,
                                                              Constants::QML_WIZARD_TR_CATEGORY));
    return parameters;
}

QWizard *QmlProjectApplicationWizard::createWizardDialog(QWidget *parent,
                                                  const QString &defaultPath,
                                                  const WizardPageList &extensionPages) const
{
    QmlProjectApplicationWizardDialog *wizard = new QmlProjectApplicationWizardDialog(parent);

    wizard->setPath(defaultPath);
    wizard->setProjectName(QmlProjectApplicationWizardDialog::uniqueProjectName(defaultPath));

    foreach (QWizardPage *p, extensionPages)
        BaseFileWizard::applyExtensionPageShortTitle(wizard, wizard->addPage(p));

    return wizard;
}

Core::GeneratedFiles QmlProjectApplicationWizard::generateFiles(const QWizard *w,
                                                     QString *errorMessage) const
{
    Q_UNUSED(errorMessage)

    const QmlProjectApplicationWizardDialog *wizard = qobject_cast<const QmlProjectApplicationWizardDialog *>(w);
    const QString projectName = wizard->projectName();
    const QString projectPath = wizard->path() + QLatin1Char('/') + projectName;

    const QString creatorFileName = Core::BaseFileWizard::buildFileName(projectPath,
                                                                        projectName,
                                                                        QLatin1String("qmlproject"));

    const QString mainFileName = Core::BaseFileWizard::buildFileName(projectPath,
                                                                     projectName,
                                                                     QLatin1String("qml"));

    QString contents;
    {
        QTextStream out(&contents);

        out
            << "import Qt 4.6" << endl
            << endl
            << "Rectangle {" << endl
            << "    width: 200" << endl
            << "    height: 200" << endl
            << "    Text {" << endl
            << "        x: 66" << endl
            << "        y: 93" << endl
            << "        text: \"Hello World\"" << endl
            << "    }" << endl
            << "}" << endl;
    }
    Core::GeneratedFile generatedMainFile(mainFileName);
    generatedMainFile.setContents(contents);

    QString projectContents;
    {
        QTextStream out(&projectContents);

        out
            //: Comment added to generated .qmlproject file
            << "/* " << tr("File generated by QtCreator", "qmlproject Template") << " */" << endl
            << endl
            << "import QmlProject 1.0" << endl
            << endl
            << "Project {" << endl
            //: Comment added to generated .qmlproject file
            << "    /* " << tr("Include .qml, .js, and image files from current directory and subdirectories", "qmlproject Template") << " */" << endl
            << "    QmlFiles {" << endl
            << "        directory: \".\"" << endl
            << "    }" << endl
            << "    JavaScriptFiles {" << endl
            << "        directory: \".\"" << endl
            << "    }" << endl
            << "    ImageFiles {" << endl
            << "        directory: \".\"" << endl
            << "    }" << endl
            //: Comment added to generated .qmlproject file
            << "    /* " << tr("List of plugin directories passed to QML runtime", "qmlproject Template") << " */" << endl
            << "    // importPaths: [ \" ../exampleplugin \" ]" << endl
            << "}" << endl;
    }
    Core::GeneratedFile generatedCreatorFile(creatorFileName);
    generatedCreatorFile.setContents(projectContents);

    Core::GeneratedFiles files;
    files.append(generatedMainFile);
    files.append(generatedCreatorFile);

    return files;
}

bool QmlProjectApplicationWizard::postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l, QString *errorMessage)
{
    Q_UNUSED(w);
    // Post-Generate: Open the project
    const QString proFileName = l.back().path();
    if (!ProjectExplorer::ProjectExplorerPlugin::instance()->openProject(proFileName)) {
        *errorMessage = tr("The project %1 could not be opened.").arg(proFileName);
        return false;
    }
    return true;
}

} // namespace Internal
} // namespace QmlProjectManager

