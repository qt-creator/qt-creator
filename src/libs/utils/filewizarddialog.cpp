/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "filewizarddialog.h"
#include "filewizardpage.h"

#include <QtGui/QAbstractButton>

namespace Core {
namespace Utils {

FileWizardDialog::FileWizardDialog(QWidget *parent) :
    QWizard(parent),
    m_filePage(new FileWizardPage)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setOption(QWizard::NoCancelButton, false);
    setOption(QWizard::NoDefaultButton, false);
    setPixmap(QWizard::WatermarkPixmap, QPixmap(QLatin1String(":/core/images/qtwatermark.png")));
    addPage(m_filePage);
    connect(m_filePage, SIGNAL(activated()), button(QWizard::FinishButton), SLOT(animateClick()));
}

QString FileWizardDialog::name() const
{
    return m_filePage->name();
}

QString FileWizardDialog::path() const
{
    return m_filePage->path();
}

void FileWizardDialog::setPath(const QString &path)
{
    m_filePage->setPath(path);

}

void FileWizardDialog::setName(const QString &name)
{
    m_filePage->setName(name);
}

} // namespace Utils
} // namespace Core
