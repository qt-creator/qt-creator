/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "textfieldcombobox.h"

#include "qtcassert.h"

namespace Utils {

/*!
    \class Utils::TextFieldComboBox
    \brief The TextFieldComboBox class is a non-editable combo box for text
    editing purposes that plays with \c QWizard::registerField (providing a
    settable 'text' property).

    Allows for a separation of values to be used for wizard fields replacement
    and display texts.
*/

TextFieldComboBox::TextFieldComboBox(QWidget *parent) :
    QComboBox(parent)
{
    setEditable(false);
    connect(this, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &TextFieldComboBox::slotCurrentIndexChanged);
}

QString TextFieldComboBox::text() const
{
    return valueAt(currentIndex());
}

void TextFieldComboBox::setText(const QString &s)
{
    const int index = findData(QVariant(s), Qt::UserRole);
    if (index != -1 && index != currentIndex())
        setCurrentIndex(index);
}

void TextFieldComboBox::slotCurrentIndexChanged(int i)
{
    emit text4Changed(valueAt(i));
}

void TextFieldComboBox::setItems(const QStringList &displayTexts,
                                 const QStringList &values)
{
    QTC_ASSERT(displayTexts.size() == values.size(), return);
    clear();
    addItems(displayTexts);
    const int count = values.count();
    for (int i = 0; i < count; i++)
        setItemData(i, QVariant(values.at(i)), Qt::UserRole);
}

QString TextFieldComboBox::valueAt(int i) const
{
    return i >= 0 && i < count() ? itemData(i, Qt::UserRole).toString() : QString();
}

} // namespace Utils
