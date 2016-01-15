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

#include "testwizarddialog.h"
#include "testwizardpage.h"
#include <projectexplorer/projectexplorerconstants.h>

#include <QFileInfo>

enum PageIds { StartPageId = 0 };

namespace QmakeProjectManager {
namespace Internal {

const char *TestWizardParameters::filePrefix = "tst_";

TestWizardParameters::TestWizardParameters() :
    type(Test),
    initializationCode(false),
    useDataSet(false),
    requiresQApplication(requiresQApplicationDefault)
{
}

TestWizardDialog::TestWizardDialog(const Core::BaseFileWizardFactory *factory,
                                   const QString &templateName,
                                   const QIcon &icon,
                                   QWidget *parent,
                                   const Core::WizardDialogParameters &parameters)  :
    BaseQmakeProjectWizardDialog(factory, true, parent, parameters),
    m_testPage(new TestWizardPage)
{
    setIntroDescription(tr("This wizard generates a Qt Unit Test "
                           "consisting of a single source file with a test class."));
    setWindowIcon(icon);
    setWindowTitle(templateName);
    setSelectedModules(QLatin1String("core testlib"), true);
    if (!parameters.extraValues().contains(QLatin1String(ProjectExplorer::Constants::PROJECT_KIT_IDS)))
        addTargetSetupPage();
    addModulesPage();
    m_testPageId = addPage(m_testPage);
    addExtensionPages(extensionPages());
    connect(this, &QWizard::currentIdChanged, this, &TestWizardDialog::slotCurrentIdChanged);
}

void TestWizardDialog::slotCurrentIdChanged(int id)
{
    if (id == m_testPageId)
        m_testPage->setProjectName(projectName());
}

TestWizardParameters TestWizardDialog::testParameters() const
{
    return m_testPage->parameters();
}

QtProjectParameters TestWizardDialog::projectParameters() const
{
    QtProjectParameters rc;
    rc.type = QtProjectParameters::ConsoleApp;
    rc.fileName = projectName();
    rc.path = path();
    // Name binary "tst_xx" after the main source
    rc.target = QFileInfo(m_testPage->sourcefileName()).baseName();
    rc.selectedModules = selectedModulesList();
    rc.deselectedModules = deselectedModulesList();
    return rc;
}

} // namespace Internal
} // namespace QmakeProjectManager
