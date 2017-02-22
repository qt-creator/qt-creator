/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "guiappwizard.h"

#include "guiappwizarddialog.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <cpptools/abstracteditorsupport.h>
#include <designer/cpp/formclasswizardparameters.h>
#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/invoker.h>
#include <qtsupport/qtsupportconstants.h>

#include <utils/fileutils.h>

#include <QCoreApplication>
#include <QDir>
#include <QTextStream>
#include <QSharedPointer>

static const char mainSourceFileC[] = "main";
static const char mainSourceShowC[] = "    w.show();\n";
static const char mainSourceMobilityShowC[] = "    w.show();\n";

static const char mainWindowUiContentsC[] =
"\n  <widget class=\"QMenuBar\" name=\"menuBar\" />"
"\n  <widget class=\"QToolBar\" name=\"mainToolBar\" />"
"\n  <widget class=\"QWidget\" name=\"centralWidget\" />"
"\n  <widget class=\"QStatusBar\" name=\"statusBar\" />";
static const char mainWindowMobileUiContentsC[] =
"\n  <widget class=\"QWidget\" name=\"centralWidget\" />";

static const char *baseClassesC[] = {"QMainWindow", "QWidget", "QDialog"};

static inline QStringList baseClasses()
{
    QStringList rc;
    const int baseClassCount = sizeof(baseClassesC)/sizeof(const char *);
    for (int i = 0; i < baseClassCount; i++)
        rc.push_back(QLatin1String(baseClassesC[i]));
    return rc;
}

namespace QmakeProjectManager {
namespace Internal {

GuiAppWizard::GuiAppWizard()
{
    setId("C.Qt4Gui");
    setCategory(QLatin1String(ProjectExplorer::Constants::QT_APPLICATION_WIZARD_CATEGORY));
    setDisplayCategory(QCoreApplication::translate("ProjectExplorer",
               ProjectExplorer::Constants::QT_APPLICATION_WIZARD_CATEGORY_DISPLAY));
    setDisplayName(tr("Qt Widgets Application"));
    setDescription(tr("Creates a Qt application for the desktop. "
                  "Includes a Qt Designer-based main window.\n\n"
                  "Preselects a desktop Qt for building the application if available."));
    setIcon(QIcon(QLatin1String(":/wizards/images/gui.png")));
    setRequiredFeatures({QtSupport::Constants::FEATURE_QWIDGETS});
}

Core::BaseFileWizard *GuiAppWizard::create(QWidget *parent, const Core::WizardDialogParameters &parameters) const
{
    GuiAppWizardDialog *dialog = new GuiAppWizardDialog(this, displayName(), icon(), parent, parameters);
    dialog->setProjectName(GuiAppWizardDialog::uniqueProjectName(parameters.defaultPath()));
    // Order! suffixes first to generate files correctly
    dialog->setLowerCaseFiles(QtWizard::lowerCaseFiles());
    dialog->setSuffixes(headerSuffix(), sourceSuffix(), formSuffix());
    dialog->setBaseClasses(baseClasses());
    return dialog;
}

// Use the class generation utils provided by the designer plugin
static inline bool generateFormClass(const GuiAppParameters &params,
                                     const Core::GeneratedFile &uiFile,
                                     Core::GeneratedFile *formSource,
                                     Core::GeneratedFile *formHeader,
                                     QString *errorMessage)
{
    // Retrieve parameters from settings
    Designer::FormClassWizardParameters fp;
    fp.uiTemplate = uiFile.contents();
    fp.uiFile = uiFile.path();
    fp.className = params.className;
    fp.sourceFile = params.sourceFileName;
    fp.headerFile = params.headerFileName;
    QString headerContents;
    QString sourceContents;
    // Invoke code generation service of Qt Designer plugin.
    if (QObject *codeGenerator = ExtensionSystem::PluginManager::getObjectByClassName(QLatin1String("Designer::QtDesignerFormClassCodeGenerator"))) {
        const QVariant code =  ExtensionSystem::invoke<QVariant>(codeGenerator, "generateFormClassCode", fp);
        if (code.type() == QVariant::List) {
            const QVariantList vl = code.toList();
            if (vl.size() == 2) {
                headerContents = vl.front().toString();
                sourceContents = vl.back().toString();
            }
        }
    }
    if (headerContents.isEmpty() || sourceContents.isEmpty()) {
        *errorMessage = QString::fromLatin1("Failed to obtain Designer plugin code generation service.");
        return false;
    }

    formHeader->setContents(headerContents);
    formSource->setContents(sourceContents);
    return true;
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
    mainSource.setContents(CppTools::AbstractEditorSupport::licenseTemplate(mainSourceFileName)
                           + contents);
    // Create files: form source with or without form
    const QString formSourceFileName = buildFileName(projectPath, params.sourceFileName, sourceSuffix());
    const QString formHeaderName = buildFileName(projectPath, params.headerFileName, headerSuffix());
    Core::GeneratedFile formSource(formSourceFileName);
    Core::GeneratedFile formHeader(formHeaderName);
    formSource.setAttributes(Core::GeneratedFile::OpenEditorAttribute);

    QSharedPointer<Core::GeneratedFile> form;
    if (params.designerForm) {
        // Create files: form
        const QString formName = buildFileName(projectPath, params.formFileName, formSuffix());
        form = QSharedPointer<Core::GeneratedFile>(new Core::GeneratedFile(formName));
        if (!parametrizeTemplate(templatePath, QLatin1String("widget.ui"), params, &contents, errorMessage))
            return Core::GeneratedFiles();
        form->setContents(contents);
        if (!generateFormClass(params, *form, &formSource, &formHeader, errorMessage))
            return Core::GeneratedFiles();
    } else {
        const QString formSourceTemplate = QLatin1String("mywidget.cpp");
        if (!parametrizeTemplate(templatePath, formSourceTemplate, params, &contents, errorMessage))
            return Core::GeneratedFiles();
        formSource.setContents(CppTools::AbstractEditorSupport::licenseTemplate(formSourceFileName)
                               + contents);
        // Create files: form header
        const QString formHeaderTemplate = QLatin1String("mywidget.h");
        if (!parametrizeTemplate(templatePath, formHeaderTemplate, params, &contents, errorMessage))
            return Core::GeneratedFiles();
        formHeader.setContents(CppTools::AbstractEditorSupport::licenseTemplate(formHeaderName)
                               + contents);
    }
    // Create files: profile
    const QString profileName = buildFileName(projectPath, projectParams.fileName, profileSuffix());
    Core::GeneratedFile profile(profileName);
    profile.setAttributes(Core::GeneratedFile::OpenProjectAttribute);
    contents.clear();
    {
        QTextStream proStr(&contents);
        QtProjectParameters::writeProFileHeader(proStr);
        projectParams.writeProFile(proStr);
        proStr << "\n\nSOURCES += " << Utils::FileName::fromString(mainSourceFileName).fileName()
               << "\\\n        " << Utils::FileName::fromString(formSource.path()).fileName()
               << "\n\nHEADERS  += " << Utils::FileName::fromString(formHeader.path()).fileName();
        if (params.designerForm)
            proStr << "\n\nFORMS    += " << Utils::FileName::fromString(form->path()).fileName();
        if (params.isMobileApplication) {
            proStr << "\n\nCONFIG += mobility"
                   << "\nMOBILITY = "
                   << "\n";
        }
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
    const QString fileName = templatePath + QLatin1Char('/') + templateName;
    Utils::FileReader reader;
    if (!reader.fetch(fileName, QIODevice::Text, errorMessage))
        return false;
    QString contents = QString::fromUtf8(reader.data());

    contents.replace(QLatin1String("%QAPP_INCLUDE%"), QLatin1String("QApplication"));
    contents.replace(QLatin1String("%INCLUDE%"), params.headerFileName);
    contents.replace(QLatin1String("%CLASS%"), params.className);
    contents.replace(QLatin1String("%BASECLASS%"), params.baseClassName);
    contents.replace(QLatin1String("%WIDGET_HEIGHT%"), QString::number(params.widgetHeight));
    contents.replace(QLatin1String("%WIDGET_WIDTH%"), QString::number(params.widgetWidth));
    if (params.isMobileApplication)
        contents.replace(QLatin1String("%SHOWMETHOD%"), QString::fromLatin1(mainSourceMobilityShowC));
    else
        contents.replace(QLatin1String("%SHOWMETHOD%"), QString::fromLatin1(mainSourceShowC));


    const QChar dot = QLatin1Char('.');

    QString preDef = params.headerFileName.toUpper();
    preDef.replace(dot, QLatin1Char('_'));
    contents.replace(QLatin1String("%PRE_DEF%"), preDef);

    const QString uiFileName = params.formFileName;
    QString uiHdr = QLatin1String("ui_");
    uiHdr += uiFileName.left(uiFileName.indexOf(dot));
    uiHdr += QLatin1String(".h");

    contents.replace(QLatin1String("%UI_HDR%"), uiHdr);
    if (params.baseClassName == QLatin1String("QMainWindow")) {
        if (params.isMobileApplication)
            contents.replace(QLatin1String("%CENTRAL_WIDGET%"), QLatin1String(mainWindowMobileUiContentsC));
        else
            contents.replace(QLatin1String("%CENTRAL_WIDGET%"), QLatin1String(mainWindowUiContentsC));
    } else {
        contents.remove(QLatin1String("%CENTRAL_WIDGET%"));
    }
    *target = contents;
    return true;
}

} // namespace Internal
} // namespace QmakeProjectManager
