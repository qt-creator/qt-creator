/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef COLORSCHEMEEDIT_H
#define COLORSCHEMEEDIT_H

#include "colorscheme.h"
#include "fontsettingspage.h"

#include <QtGui/QDialog>

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

namespace TextEditor {
namespace Internal {

namespace Ui {
class ColorSchemeEdit;
}

class FormatsModel;

/*!
  A widget for editing a color scheme. Used in the FontSettingsPage.
  */
class ColorSchemeEdit : public QWidget
{
    Q_OBJECT

public:
    ColorSchemeEdit(QWidget *parent = 0);
    ~ColorSchemeEdit();

    void setFormatDescriptions(const FormatDescriptions &descriptions);
    void setBaseFont(const QFont &font);
    void setReadOnly(bool readOnly);

    void setColorScheme(const ColorScheme &colorScheme);
    const ColorScheme &colorScheme() const;

private slots:
    void currentItemChanged(const QModelIndex &index);
    void changeForeColor();
    void changeBackColor();
    void eraseBackColor();
    void checkCheckBoxes();

private:
    void updateControls();
    void setItemListBackground(const QColor &color);

    TextEditor::FormatDescriptions m_descriptions;
    ColorScheme m_scheme;
    int m_curItem;
    Ui::ColorSchemeEdit *m_ui;
    FormatsModel *m_formatsModel;
    bool m_readOnly;
};


} // namespace Internal
} // namespace TextEditor

#endif // COLORSCHEMEEDIT_H
