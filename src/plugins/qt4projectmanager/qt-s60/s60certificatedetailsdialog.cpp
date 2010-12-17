/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "s60certificatedetailsdialog.h"
#include "ui_s60certificatedetailsdialog.h"

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

struct S60CertificateDetailsDialogPrivate
{
    S60CertificateDetailsDialogPrivate(){};
    Ui::S60CertificateDetailsDialog m_ui;
};

S60CertificateDetailsDialog::S60CertificateDetailsDialog(QWidget *parent) :
    QDialog(parent),
    m_d(new S60CertificateDetailsDialogPrivate)
{
    m_d->m_ui.setupUi(this);
    connect(m_d->m_ui.buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(close()));
}

S60CertificateDetailsDialog::~S60CertificateDetailsDialog()
{
    delete m_d;
}

void S60CertificateDetailsDialog::setText(const QString &text)
{
    m_d->m_ui.textBrowser->setText(text);
}
