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

#include "filewizarddialog.h"
#include "filewizardpage.h"

#include "hostosinfo.h"

#include <QAbstractButton>

/*!
  \class Utils::FileWizardDialog

  \brief Standard wizard for a single file letting the user choose name
   and path. Custom pages can be added via Core::IWizardExtension.
*/

namespace Utils {

FileWizardDialog::FileWizardDialog(QWidget *parent) :
    Wizard(parent),
    m_filePage(new FileWizardPage)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setOption(QWizard::NoCancelButton, false);
    setOption(QWizard::NoDefaultButton, false);
    if (HostOsInfo::isMacHost()) {
        setButtonLayout(QList<QWizard::WizardButton>()
                        << QWizard::CancelButton
                        << QWizard::Stretch
                        << QWizard::BackButton
                        << QWizard::NextButton
                        << QWizard::CommitButton
                        << QWizard::FinishButton);
    }
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

bool FileWizardDialog::forceFirstCapitalLetterForFileName() const
{
    return m_filePage->forceFirstCapitalLetterForFileName();
}

void FileWizardDialog::setForceFirstCapitalLetterForFileName(bool b)
{
    m_filePage->setForceFirstCapitalLetterForFileName(b);
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
