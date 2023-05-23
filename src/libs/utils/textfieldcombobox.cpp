// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "textfieldcombobox.h"

#include "qtcassert.h"

namespace Utils {

/*!
    \class Utils::TextFieldComboBox
    \inmodule QtCreator
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
    connect(this, &QComboBox::currentIndexChanged,
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
