/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "filewizarddialog.h"
#include "filewizardpage.h"

#include <QtGui/QAbstractButton>

namespace Utils {

FileWizardDialog::FileWizardDialog(QWidget *parent) :
    Wizard(parent),
    m_filePage(new FileWizardPage)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setOption(QWizard::NoCancelButton, false);
    setOption(QWizard::NoDefaultButton, false);
    const int filePageId = addPage(m_filePage);
    wizardProgress()->item(filePageId)->setTitle(tr("Location"));
    connect(m_filePage, SIGNAL(activated()), button(QWizard::FinishButton), SLOT(animateClick()));
}

QString FileWizardDialog::fileName() const
{
    return m_filePage->fileName();
}

QString FileWizardDialog::path() const
{
    return m_filePage->path();
}

void FileWizardDialog::setPath(const QString &path)
{
    m_filePage->setPath(path);

}

void FileWizardDialog::setFileName(const QString &name)
{
    m_filePage->setFileName(name);
}

} // namespace Utils
