/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** 
** Non-Open Source Usage  
** 
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.  
** 
** GNU General Public License Usage 
** 
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
** 
***************************************************************************/

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
    setPixmap(QWizard::WatermarkPixmap, QPixmap(QLatin1String(":/qworkbench/images/qtwatermark.png")));
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
