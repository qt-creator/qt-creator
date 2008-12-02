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

#include <QtGui/QPushButton>

#include "filternamedialog.h"

QT_BEGIN_NAMESPACE

FilterNameDialog::FilterNameDialog(QWidget *parent)
    : QDialog(parent)
{
    m_ui.setupUi(this);
    connect(m_ui.buttonBox->button(QDialogButtonBox::Ok),
        SIGNAL(clicked()), this, SLOT(accept()));
    connect(m_ui.buttonBox->button(QDialogButtonBox::Cancel),
        SIGNAL(clicked()), this, SLOT(reject()));
    connect(m_ui.lineEdit, SIGNAL(textChanged(const QString&)),
        this, SLOT(updateOkButton()));
    m_ui.buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);

}

QString FilterNameDialog::filterName() const
{
    return m_ui.lineEdit->text();
}

void FilterNameDialog::updateOkButton()
{
    m_ui.buttonBox->button(QDialogButtonBox::Ok)
        ->setDisabled(m_ui.lineEdit->text().isEmpty());
}

QT_END_NAMESPACE
