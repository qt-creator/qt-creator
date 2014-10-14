/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "pythonclasswizard.h"
#include "pythonclasswizarddialog.h"
#include "pythonclassnamepage.h"
#include "pythonsourcegenerator.h"
#include "../pythoneditorconstants.h"
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/session.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/baseqtversion.h>

using namespace ProjectExplorer;

namespace PythonEditor {
namespace Internal {

ClassWizard::ClassWizard()
{
    setWizardKind(Core::IWizardFactory::FileWizard);
    setId(QLatin1String(Constants::C_PY_CLASS_WIZARD_ID));
    setCategory(QLatin1String(Constants::C_PY_WIZARD_CATEGORY));
    setDisplayCategory(QLatin1String(Constants::C_PY_DISPLAY_CATEGORY));
    setDisplayName(ClassWizard::tr(Constants::EN_PY_CLASS_DISPLAY_NAME));
    setDescription(ClassWizard::tr(Constants::EN_PY_CLASS_DESCRIPTION));
}

Core::BaseFileWizard *ClassWizard::create(QWidget *parent, const Core::WizardDialogParameters &parameters) const
{
    ClassWizardDialog *wizard = new ClassWizardDialog(parent);
    foreach (QWizardPage *p, parameters.extensionPages())
        wizard->addPage(p);
    wizard->setPath(parameters.defaultPath());
    wizard->setExtraValues(parameters.extraValues());
    return wizard;
}

Core::GeneratedFiles ClassWizard::generateFiles(const QWizard *w,
                                                QString *errorMessage) const
{
    Q_UNUSED(errorMessage);

    const ClassWizardDialog *wizard = qobject_cast<const ClassWizardDialog *>(w);
    const ClassWizardParameters params = wizard->parameters();

    const QString fileName = Core::BaseFileWizardFactory::buildFileName(
                params.path, params.fileName, QLatin1String(Constants::C_PY_EXTENSION));
    Core::GeneratedFile sourceFile(fileName);

    SourceGenerator generator;
    generator.setPythonQtBinding(SourceGenerator::PySide);
    Kit *kit = kitForWizard(wizard);
    if (kit) {
        QtSupport::BaseQtVersion *baseVersion = QtSupport::QtKitInformation::qtVersion(kit);
        if (baseVersion && baseVersion->qtVersion().majorVersion == 5)
            generator.setPythonQtVersion(SourceGenerator::Qt5);
        else
            generator.setPythonQtVersion(SourceGenerator::Qt4);
    }

    QString sourceContent = generator.generateClass(
                params.className, params.baseClass, params.classType
                );

    sourceFile.setContents(sourceContent);
    sourceFile.setAttributes(Core::GeneratedFile::OpenEditorAttribute);
    return Core::GeneratedFiles() << sourceFile;
}

Kit *ClassWizard::kitForWizard(const ClassWizardDialog *wizard) const
{
    const QString key = QLatin1String(ProjectExplorer::Constants::PREFERRED_PROJECT_NODE);
    const QString nodePath = wizard->extraValues().value(key).toString();

    // projectForFile doesn't find project if project file path passed
    Node *node = SessionManager::nodeForFile(nodePath);
    Project *proj = SessionManager::projectForNode(node);
    if (proj && proj->activeTarget())
        return proj->activeTarget()->kit();

    return KitManager::defaultKit();
}

} // namespace Internal
} // namespace PythonEditor
