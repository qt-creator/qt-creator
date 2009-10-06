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

#include "customwidgetwizard.h"
#include "customwidgetwizarddialog.h"
#include "plugingenerator.h"
#include "pluginoptions.h"
#include "filenamingparameters.h"

#include <cpptools/cppmodelmanagerinterface.h>
#include <utils/pathchooser.h>

namespace Qt4ProjectManager {
namespace Internal {

CustomWidgetWizard::CustomWidgetWizard() :
    QtWizard(tr("Qt4 Designer Custom Widget"),
             tr("Creates a Qt4 Designer Custom Widget or a Custom Widget Collection."),
             QIcon(":/wizards/images/gui.png"))
{
}

QWizard *CustomWidgetWizard::createWizardDialog(QWidget *parent,
                                                const QString &defaultPath,
                                                const WizardPageList &extensionPages) const
{
    CustomWidgetWizardDialog *rc = new CustomWidgetWizardDialog(name(), icon(), extensionPages, parent);
    rc->setPath(defaultPath.isEmpty() ? Utils::PathChooser::homePath() : defaultPath);
    rc->setFileNamingParameters(FileNamingParameters(headerSuffix(), sourceSuffix(), QtWizard::lowerCaseFiles()));
    return rc;
}

Core::GeneratedFiles CustomWidgetWizard::generateFiles(const QWizard *w,
                                                       QString *errorMessage) const
{
    const CustomWidgetWizardDialog *cw = qobject_cast<const CustomWidgetWizardDialog *>(w);
    Q_ASSERT(w);
    GenerationParameters p;
    p.name = cw->name();
    p.path = cw->path();
    p.license = CppTools::AbstractEditorSupport::licenseTemplate();
    p.templatePath = QtWizard::templateDir();
    p.templatePath += QLatin1String("/customwidgetwizard");
    return PluginGenerator::generatePlugin(p, *(cw->pluginOptions()), errorMessage);
}

} // namespace Internal
} // namespace Qt4ProjectManager
