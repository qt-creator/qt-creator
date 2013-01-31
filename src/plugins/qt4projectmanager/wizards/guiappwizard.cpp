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

#include "guiappwizard.h"

#include "guiappwizarddialog.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <cpptools/abstracteditorsupport.h>
#include <designer/cpp/formclasswizardparameters.h>
#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/invoker.h>
#include <qtsupport/qtsupportconstants.h>

#include <utils/fileutils.h>

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QUuid>
#include <QFileInfo>
#include <QSharedPointer>

#include <QIcon>

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
    : QtWizard(QLatin1String("C.Qt4Gui"),
               QLatin1String(ProjectExplorer::Constants::QT_APPLICATION_WIZARD_CATEGORY),
               QLatin1String(ProjectExplorer::Constants::QT_APPLICATION_WIZARD_CATEGORY_DISPLAY),
               tr("Qt Gui Application"),
               tr("Creates a Qt application for the desktop. "
                  "Includes a Qt Designer-based main window.\n\n"
                  "Preselects a desktop Qt for building the application if available."),
               QIcon(QLatin1String(":/wizards/images/gui.png"))),
      m_createMobileProject(false)
{
}

Core::FeatureSet GuiAppWizard::requiredFeatures() const
{
    return Core::Feature(QtSupport::Constants::FEATURE_QWIDGETS);
}

GuiAppWizard::GuiAppWizard(const QString &id,
                           const QString &category,
                           const QString &displayCategory,
                           const QString &name,
                           const QString &description,
                           const QIcon &icon,
                           bool createMobile)
    : QtWizard(id, category, displayCategory, name, description, icon),
      m_createMobileProject(createMobile)
{
}

QWizard *GuiAppWizard::createWizardDialog(QWidget *parent,
                                          const Core::WizardDialogParameters &wizardDialogParameters) const
{
    GuiAppWizardDialog *dialog = new GuiAppWizardDialog(displayName(), icon(),
                                                        showModulesPageForApplications(),
                                                        m_createMobileProject,
                                                        parent, wizardDialogParameters);
    dialog->setProjectName(GuiAppWizardDialog::uniqueProjectName(wizardDialogParameters.defaultPath()));
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
    const QString license = CppTools::AbstractEditorSupport::licenseTemplate();

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
        proStr << "\n\nSOURCES += " << QFileInfo(mainSourceFileName).fileName()
               << "\\\n        " << QFileInfo(formSource.path()).fileName()
               << "\n\nHEADERS  += " << QFileInfo(formHeader.path()).fileName();
        if (params.designerForm)
            proStr << "\n\nFORMS    += " << QFileInfo(form->path()).fileName();
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
    QString fileName = templatePath;
    fileName += QDir::separator();
    fileName += templateName;
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
} // namespace Qt4ProjectManager
