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

#include "testwizarddialog.h"
#include "testwizardpage.h"

#include <QtCore/QFileInfo>

enum PageIds { StartPageId = 0 };

namespace Qt4ProjectManager {
namespace Internal {

const char *TestWizardParameters::filePrefix = "tst_";

TestWizardParameters::TestWizardParameters() :
    type(Test),
    initializationCode(false),
    useDataSet(false),
    requiresQApplication(requiresQApplicationDefault)
{
}

TestWizardDialog::TestWizardDialog(const QString &templateName,
                                   const QIcon &icon,
                                   const QList<QWizardPage*> &extensionPages,
                                   QWidget *parent)  :
    BaseQt4ProjectWizardDialog(true, parent),
    m_testPage(new TestWizardPage),
    m_testPageId(-1), m_modulesPageId(-1)
{
    setIntroDescription(tr("This wizard generates a Qt unit test "
                           "consisting of a single source file with a test class."));
    setWindowIcon(icon);
    setWindowTitle(templateName);
    setSelectedModules(QLatin1String("core testlib"), true);
    addTargetSetupPage();
    m_modulesPageId = addModulesPage();
    m_testPageId = addPage(m_testPage);
    wizardProgress()->item(m_testPageId)->setTitle(tr("Details"));
    foreach (QWizardPage *p, extensionPages)
        Core::BaseFileWizard::applyExtensionPageShortTitle(this, addPage(p));
    connect(this, SIGNAL(currentIdChanged(int)), this, SLOT(slotCurrentIdChanged(int)));
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
    rc.selectedModules = selectedModules();
    rc.deselectedModules = deselectedModules();
    return rc;
}

} // namespace Internal
} // namespace Qt4ProjectManager
