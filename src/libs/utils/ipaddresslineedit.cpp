/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "ipaddresslineedit.h"

#include <QtGui/QRegExpValidator>

namespace Utils {

// ------------------ IpAddressLineEditPrivate

class IpAddressLineEditPrivate
{
public:
    IpAddressLineEditPrivate();

    QValidator *m_ipAddressValidator;
    QColor m_validColor;
    bool m_addressIsValid;
};

IpAddressLineEditPrivate::IpAddressLineEditPrivate() :
    m_addressIsValid(true)
{
}

IpAddressLineEdit::IpAddressLineEdit(QWidget* parent) :
    QLineEdit(parent),
    m_d(new IpAddressLineEditPrivate())
{
    m_d->m_validColor = palette().color(QPalette::Text);

    const char * ipAddressRegExpPattern = "^\\b(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\."
            "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\."
            "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\."
            "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\b"
            "((:)(6553[0-5]|655[0-2]\\d|65[0-4]\\d\\d|6[0-4]\\d{3}|[1-5]\\d{4}|[1-9]\\d{0,3}|0))?$";

    QRegExp ipAddressRegExp(ipAddressRegExpPattern);
    m_d->m_ipAddressValidator = new QRegExpValidator(ipAddressRegExp, this);

    connect(this, SIGNAL(textChanged(QString)), this, SLOT(validateAddress(QString)));
}

IpAddressLineEdit::~IpAddressLineEdit()
{
    delete m_d;
}

bool IpAddressLineEdit::isValid() const
{
    return m_d->m_addressIsValid;
}

void IpAddressLineEdit::validateAddress(const QString &string)
{
    QString copy = string;
    int offset = 0;
    bool isValid = m_d->m_ipAddressValidator->validate(copy, offset) == QValidator::Acceptable;

    if (isValid != m_d->m_addressIsValid) {
        if (isValid) {
            QPalette pal(palette());
            pal.setColor(QPalette::Text, m_d->m_validColor);
            setPalette(pal);
            emit validAddressChanged(copy);
        } else {
            QPalette pal(palette());
            pal.setColor(QPalette::Text, Qt::red);
            setPalette(pal);
            setToolTip(tr("The IP address is not valid."));
        }
        m_d->m_addressIsValid = isValid;
    } else {
        if (isValid)
            emit validAddressChanged(copy);
        else
            emit invalidAddressChanged();
    }
}

} // namespace Utils
