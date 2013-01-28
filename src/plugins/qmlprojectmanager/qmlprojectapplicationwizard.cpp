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

#include "qmlprojectapplicationwizard.h"

#include "qmlprojectconstants.h"

#include <app/app_version.h>
#include <projectexplorer/customwizard/customwizard.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <qtsupport/qtsupportconstants.h>

#include <QIcon>

#include <QPainter>
#include <QPixmap>

#include <QDir>
#include <QTextStream>
#include <QCoreApplication>

namespace QmlProjectManager {
namespace Internal {

QmlProjectApplicationWizardDialog::QmlProjectApplicationWizardDialog(QWidget *parent,
                                                                     const Core::WizardDialogParameters &parameters) :
    ProjectExplorer::BaseProjectWizardDialog(parent, parameters)
{
    setWindowTitle(tr("New Qt Quick UI Project"));
    setIntroDescription(tr("This wizard generates a Qt Quick UI project."));
}

QmlProjectApplicationWizard::QmlProjectApplicationWizard(ProjectType projectType)
    : Core::BaseFileWizard(parameters(projectType)), m_projectType(projectType)
{ }

QmlProjectApplicationWizard::~QmlProjectApplicationWizard()
{ }

Core::FeatureSet QmlProjectApplicationWizard::requiredFeatures() const
{
    switch (m_projectType) {
    case QtQuick2Project:
        return Core::Feature(QtSupport::Constants::FEATURE_QT_QUICK_2)
                | Core::Feature(QtSupport::Constants::FEATURE_QMLPROJECT);
    case QtQuick1Project:
    default:
        return Core::Feature(QtSupport::Constants::FEATURE_QMLPROJECT)
                | Core::Feature(QtSupport::Constants::FEATURE_QT_QUICK_1_1);
    }
}

Core::BaseFileWizardParameters QmlProjectApplicationWizard::parameters(ProjectType projectType)
{
    Core::BaseFileWizardParameters parameters(ProjectWizard);
    parameters.setIcon(QIcon(QLatin1String(QtSupport::Constants::QML_WIZARD_ICON)));
    switch (projectType) {
    case QtQuick2Project:
        parameters.setDisplayName(tr("Qt Quick 2 UI"));
        parameters.setId(QLatin1String("QB.QML Application for Qt Quick 2.0"));

        parameters.setDescription(tr("Creates a Qt Quick UI 2 project with a single "
                                     "QML file that contains the main view.\n\n"
                                     "You can review Qt Quick UI 2 projects in the QML Scene and you need not build them. "
                                     "You do not need to have the development environment installed "
                                     "on your computer to create and run this type of projects.\n\nRequires <b>Qt 5.0</b> or newer."));
        break;
    case QtQuick1Project:
    default:
        parameters.setDisplayName(tr("Qt Quick 1 UI"));
        parameters.setId(QLatin1String("QB.QML Application for Qt Quick 1.1"));

        parameters.setDescription(tr("Creates a Qt Quick UI 1 project with a single "
                                     "QML file that contains the main view.\n\n"
                                     "You can review Qt Quick UI 1 projects in the QML Viewer and you need not build them. "
                                     "You do not need to have the development environment installed "
                                     "on your computer to create and run this type of projects.\n\nRequires <b>Qt 4.8</b> or newer."));

    }

    parameters.setCategory(QLatin1String(ProjectExplorer::Constants::QT_APPLICATION_WIZARD_CATEGORY));
    parameters.setDisplayCategory(QT_TRANSLATE_NOOP("ProjectExplorer", "Qt Application"));
    return parameters;
}

QWizard *QmlProjectApplicationWizard::createWizardDialog(QWidget *parent,
                                                         const Core::WizardDialogParameters &wizardDialogParameters) const
{
    QmlProjectApplicationWizardDialog *wizard = new QmlProjectApplicationWizardDialog(parent, wizardDialogParameters);

    wizard->setProjectName(QmlProjectApplicationWizardDialog::uniqueProjectName(wizardDialogParameters.defaultPath()));

    wizard->addExtensionPages(wizardDialogParameters.extensionPages());

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

        switch (m_projectType) {
        case QtQuick2Project:
            out << "import QtQuick 2.0" << endl;
            break;
        case QtQuick1Project:
        default:
            out << "import QtQuick 1.1" << endl;
        }

        out
            << endl
            << "Rectangle {" << endl
            << "    width: 360" << endl
            << "    height: 360" << endl
            << "    Text {" << endl
            << "        anchors.centerIn: parent" << endl
            << "        text: \"Hello World\"" << endl
            << "    }" << endl
            << "    MouseArea {" << endl
            << "        anchors.fill: parent" << endl
            << "        onClicked: {" << endl
            << "            Qt.quit();" << endl
            << "        }" << endl
            << "    }" << endl
            << "}" << endl;
    }
    Core::GeneratedFile generatedMainFile(mainFileName);
    generatedMainFile.setContents(contents);
    generatedMainFile.setAttributes(Core::GeneratedFile::OpenEditorAttribute);

    QString projectContents;
    {
        QTextStream out(&projectContents);

        out << "/* File generated by Qt Creator, version " << Core::Constants::IDE_VERSION_LONG << " */" << endl
            << endl
            << "import QmlProject 1.1" << endl
            << endl
            << "Project {" << endl
            << "    mainFile: \"" << QDir(projectPath).relativeFilePath(mainFileName) << '"' << endl
            << endl
            << "    /* Include .qml, .js, and image files from current directory and subdirectories */" << endl
            << "    QmlFiles {" << endl
            << "        directory: \".\"" << endl
            << "    }" << endl
            << "    JavaScriptFiles {" << endl
            << "        directory: \".\"" << endl
            << "    }" << endl
            << "    ImageFiles {" << endl
            << "        directory: \".\"" << endl
            << "    }" << endl
            << "    /* List of plugin directories passed to QML runtime */" << endl
            << "    // importPaths: [ \"../exampleplugin\" ]" << endl
            << "}" << endl;
    }
    Core::GeneratedFile generatedCreatorFile(creatorFileName);
    generatedCreatorFile.setContents(projectContents);
    generatedCreatorFile.setAttributes(Core::GeneratedFile::OpenProjectAttribute);

    Core::GeneratedFiles files;
    files.append(generatedMainFile);
    files.append(generatedCreatorFile);

    return files;
}

bool QmlProjectApplicationWizard::postGenerateFiles(const QWizard *, const Core::GeneratedFiles &l, QString *errorMessage)
{
    return ProjectExplorer::CustomProjectWizard::postGenerateOpen(l, errorMessage);

}

} // namespace Internal
} // namespace QmlProjectManager

