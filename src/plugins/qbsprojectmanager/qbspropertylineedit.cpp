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

#include "qbspropertylineedit.h"

#include <utils/qtcprocess.h>

#include <QStringList>

namespace QbsProjectManager {
namespace Internal {

QbsPropertyLineEdit::QbsPropertyLineEdit(QWidget *parent) :
    Utils::BaseValidatingLineEdit(parent)
{ }

QList<QPair<QString, QString> > QbsPropertyLineEdit::properties() const
{
    return m_propertyCache;
}

bool QbsPropertyLineEdit::validate(const QString &value, QString *errorMessage) const
{
    Utils::QtcProcess::SplitError err;
    QStringList argList = Utils::QtcProcess::splitArgs(value, false, &err);
    if (err != Utils::QtcProcess::SplitOk) {
        if (errorMessage)
            *errorMessage = tr("Could not split properties.");
        return false;
    }

    QList<QPair<QString, QString> > properties;
    foreach (const QString &arg, argList) {
        int pos = arg.indexOf(QLatin1Char(':'));
        QString key;
        QString value;
        if (pos > 0) {
            key = arg.left(pos);
            value = arg.mid(pos + 1);
            properties.append(qMakePair(key, value));
        } else {
            if (errorMessage)
                *errorMessage = tr("No ':' found in property definition.");
            return false;
        }
    }

    if (m_propertyCache != properties) {
        m_propertyCache = properties;
        emit propertiesChanged();
    }
    return true;
}

} // namespace Internal
} // namespace QbsProjectManager

