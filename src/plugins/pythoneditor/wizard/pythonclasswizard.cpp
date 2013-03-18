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

static Core::BaseFileWizardParameters getDefaultParams()
{
    Core::BaseFileWizardParameters p(Core::IWizard::FileWizard);

    p.setId(QLatin1String(Constants::C_PY_CLASS_WIZARD_ID));
    p.setCategory(QLatin1String(Constants::C_PY_WIZARD_CATEGORY));
    p.setDisplayCategory(QLatin1String(Constants::C_PY_DISPLAY_CATEGORY));
    p.setDisplayName(ClassWizard::tr(Constants::EN_PY_CLASS_DISPLAY_NAME));
    p.setDescription(ClassWizard::tr(Constants::EN_PY_CLASS_DESCRIPTION));

    return p;
}

ClassWizard::ClassWizard(QObject *parent) :
    Core::BaseFileWizard(getDefaultParams(), parent)
{
}

QWizard *ClassWizard::createWizardDialog(
        QWidget *parent,
        const Core::WizardDialogParameters &params) const
{
    ClassWizardDialog *wizard = new ClassWizardDialog(parent);
    foreach (QWizardPage *p, params.extensionPages())
        BaseFileWizard::applyExtensionPageShortTitle(wizard, wizard->addPage(p));
    wizard->setPath(params.defaultPath());
    wizard->setExtraValues(params.extraValues());
    return wizard;
}

Core::GeneratedFiles ClassWizard::generateFiles(const QWizard *w,
                                                QString *errorMessage) const
{
    Q_UNUSED(errorMessage);

    const ClassWizardDialog *wizard = qobject_cast<const ClassWizardDialog *>(w);
    const ClassWizardParameters params = wizard->parameters();

    const QString fileName = Core::BaseFileWizard::buildFileName(
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
    const QString key = QLatin1String(ProjectExplorer::Constants::PREFERED_PROJECT_NODE);
    const QString nodePath = wizard->extraValues().value(key).toString();

    // projectForFile doesn't find project if project file path passed
    Node *node = ProjectExplorerPlugin::instance()->session()->nodeForFile(nodePath);
    Project *proj = ProjectExplorerPlugin::instance()->session()->projectForNode(node);
    if (proj && proj->activeTarget())
        return proj->activeTarget()->kit();

    return KitManager::instance()->defaultKit();
}

} // namespace PythonEditor
