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

#include "emptyprojectwizarddialog.h"

#include "emptyprojectwizard.h"

#include <utils/projectintropage.h>

namespace Qt4ProjectManager {
namespace Internal {

EmptyProjectWizardDialog::EmptyProjectWizardDialog(const QString &templateName,
                                               const QIcon &icon,
                                               const QList<QWizardPage*> &extensionPages,
                                               QWidget *parent) :
    QWizard(parent),
    m_introPage(new Utils::ProjectIntroPage)
{
    setWindowIcon(icon);
    setWindowTitle(templateName);
    Core::BaseFileWizard::setupWizard(this);

    m_introPage->setDescription(tr("This wizard generates an empty Qt4 project. "
                          "Add files to it later on by using the other wizards."));

    addPage(m_introPage);

    foreach (QWizardPage *p, extensionPages)
        addPage(p);
}

void EmptyProjectWizardDialog::setPath(const QString &path)
{
    m_introPage->setPath(path);
}

void  EmptyProjectWizardDialog::setName(const QString &name)
{
    m_introPage->setName(name);
}

QtProjectParameters EmptyProjectWizardDialog::parameters() const
{
    QtProjectParameters rc;
    rc.type = QtProjectParameters::EmptyProject;
    rc.name = m_introPage->name();
    rc.path = m_introPage->path();
    return rc;
}

} // namespace Internal
} // namespace Qt4ProjectManager
