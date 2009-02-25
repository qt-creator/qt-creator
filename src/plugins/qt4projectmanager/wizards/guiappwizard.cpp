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

#include "guiappwizard.h"
#include "guiappwizarddialog.h"
#include "qt4projectmanager.h"
#include "modulespage.h"
#include "filespage.h"
#include "qt4projectmanagerconstants.h"

#include <utils/pathchooser.h>
#include <projectexplorer/projectnodes.h>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QByteArray>
#include <QtCore/QDebug>
#include <QtCore/QTextStream>
#include <QtCore/QFileInfo>
#include <QtCore/QSharedPointer>

#include <QtGui/QIcon>

static const char *mainSourceFileC = "main";
static const char *mainWindowUiContentsC =
"\n  <widget class=\"QMenuBar\" name=\"menuBar\" />"
"\n  <widget class=\"QToolBar\" name=\"mainToolBar\" />"
"\n  <widget class=\"QWidget\" name=\"centralWidget\" />"
"\n  <widget class=\"QStatusBar\" name=\"statusBar\" />";

static const char *baseClassesC[] = { "QMainWindow", "QWidget", "QDialog" };

static inline QStringList baseClasses()
{
    QStringList rc;
    const int baseClassCount = sizeof(baseClassesC)/sizeof(const char *);
    for (int i = 0; i < baseClassCount; i++)
        rc.push_back(QLatin1String(baseClassesC[i]));
    return rc;
}

namespace Qt4ProjectManager {
namespace Internal {

GuiAppWizard::GuiAppWizard()
  : QtWizard(tr("Qt4 Gui Application"),
             tr("Creates a Qt4 Gui Application with one form."),
             QIcon(":/wizards/images/gui.png"))
{
}

QWizard *GuiAppWizard::createWizardDialog(QWidget *parent,
                                          const QString &defaultPath,
                                          const WizardPageList &extensionPages) const
{
    GuiAppWizardDialog *dialog = new GuiAppWizardDialog(name(), icon(), extensionPages, parent);
    dialog->setPath(defaultPath.isEmpty() ? Core::Utils::PathChooser::homePath() : defaultPath);
    // Order! suffixes first to generate files correctly
    dialog->setSuffixes(headerSuffix(), sourceSuffix(), formSuffix());
    dialog->setBaseClasses(baseClasses());
    return dialog;
}

Core::GeneratedFiles GuiAppWizard::generateFiles(const QWizard *w,
                                                 QString *errorMessage) const
{
    const GuiAppWizardDialog *dialog = qobject_cast<const GuiAppWizardDialog *>(w);
    const QtProjectParameters projectParams = dialog->projectParameters();
    const QString projectPath = projectParams.projectPath();
    const GuiAppParameters params = dialog->parameters();

    // Generate file names. Note that the path for the project files is the
    // newly generated project directory.
    const QString templatePath = templateDir();
    // Create files: main source
    QString contents;
    const QString mainSourceFileName = buildFileName(projectPath, QLatin1String(mainSourceFileC), sourceSuffix());
    Core::GeneratedFile mainSource(mainSourceFileName);
    if (!parametrizeTemplate(templatePath, QLatin1String("main.cpp"), params, &contents, errorMessage))
        return Core::GeneratedFiles();
    mainSource.setContents(contents);
    // Create files: form source
    const QString formSourceTemplate = params.designerForm ? QLatin1String("mywidget_form.cpp") : QLatin1String("mywidget.cpp");
    const QString formSourceFileName = buildFileName(projectPath, params.sourceFileName, sourceSuffix());
    Core::GeneratedFile formSource(formSourceFileName);
    if (!parametrizeTemplate(templatePath, formSourceTemplate, params, &contents, errorMessage))
        return Core::GeneratedFiles();
    formSource.setContents(contents);
    // Create files: form header
    const QString formHeaderName = buildFileName(projectPath, params.headerFileName, headerSuffix());
    const QString formHeaderTemplate = params.designerForm ? QLatin1String("mywidget_form.h") : QLatin1String("mywidget.h");
    Core::GeneratedFile formHeader(formHeaderName);
    if (!parametrizeTemplate(templatePath, formHeaderTemplate, params, &contents, errorMessage))
        return Core::GeneratedFiles();
    formHeader.setContents(contents);
    // Create files: form
    QSharedPointer<Core::GeneratedFile> form;
    if (params.designerForm) {
        const QString formName = buildFileName(projectPath, params.formFileName, formSuffix());
        form = QSharedPointer<Core::GeneratedFile>(new Core::GeneratedFile(formName));
        if (!parametrizeTemplate(templatePath, QLatin1String("widget.ui"), params, &contents, errorMessage))
            return Core::GeneratedFiles();
        form->setContents(contents);
    }
    // Create files: profile
    const QString profileName = buildFileName(projectPath, projectParams.name, profileSuffix());
    Core::GeneratedFile profile(profileName);
    contents.clear();
    {
        QTextStream proStr(&contents);
        QtProjectParameters::writeProFileHeader(proStr);
        projectParams.writeProFile(proStr);
        proStr << "\n\nSOURCES += " << QFileInfo(mainSourceFileName).fileName()
               << "\\\n        " << QFileInfo(formSource.path()).fileName()
               << "\n\nHEADERS  += " << QFileInfo(formHeader.path()).fileName();
        if (params.designerForm)
            proStr << "\n\nFORMS    += " << QFileInfo(form->path()).fileName();
        proStr << '\n';
    }
    profile.setContents(contents);
    // List
    Core::GeneratedFiles rc;
    rc << mainSource << formSource << formHeader;
    if (params.designerForm)
        rc << *form;
    rc << profile;
    return rc;
}

bool GuiAppWizard::parametrizeTemplate(const QString &templatePath, const QString &templateName,
                                       const GuiAppParameters &params,
                                       QString *target, QString *errorMessage)
{
    QString fileName = templatePath;
    fileName += QDir::separator();
    fileName += templateName;
    QFile inFile(fileName);
    if (!inFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        *errorMessage = tr("The template file '%1' could not be opened for reading: %2").arg(fileName, inFile.errorString());
        return false;
    }
    QString contents = QString::fromUtf8(inFile.readAll());

    contents.replace(QLatin1String("%QAPP_INCLUDE%"), QLatin1String("QtGui/QApplication"));
    contents.replace(QLatin1String("%INCLUDE%"), params.headerFileName);
    contents.replace(QLatin1String("%CLASS%"), params.className);
    contents.replace(QLatin1String("%BASECLASS%"), params.baseClassName);

    const QChar dot = QLatin1Char('.');

    QString preDef = params.headerFileName.toUpper();
    preDef.replace(dot, QLatin1Char('_'));
    contents.replace("%PRE_DEF%", preDef.toUtf8());

    const QString uiFileName = params.formFileName;
    QString uiHdr = QLatin1String("ui_");
    uiHdr += uiFileName.left(uiFileName.indexOf(dot));
    uiHdr += QLatin1String(".h");

    contents.replace(QLatin1String("%UI_HDR%"), uiHdr);
    if (params.baseClassName == QLatin1String("QMainWindow")) {
        contents.replace(QLatin1String("%CENTRAL_WIDGET%"), QLatin1String(mainWindowUiContentsC));
    } else {
        contents.remove(QLatin1String("%CENTRAL_WIDGET%"));
    }
    *target = contents;
    return true;
}

} // namespace Internal
} // namespace Qt4ProjectManager
