/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "colorscheme.h"
#include "fontsettingspage.h"

#include <QDialog>

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

namespace TextEditor {
namespace Internal {

namespace Ui { class ColorSchemeEdit; }

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

private:
    void currentItemChanged(const QModelIndex &index);
    void changeForeColor();
    void changeBackColor();
    void eraseForeColor();
    void eraseBackColor();
    void changeRelativeForeColor();
    void changeRelativeBackColor();
    void eraseRelativeForeColor();
    void eraseRelativeBackColor();
    void checkCheckBoxes();
    void changeUnderlineColor();
    void eraseUnderlineColor();
    void changeUnderlineStyle(int index);

    void updateControls();
    void updateForegroundControls();
    void updateBackgroundControls();
    void updateRelativeForegroundControls();
    void updateRelativeBackgroundControls();
    void updateFontControls();
    void updateUnderlineControls();
    void setItemListBackground(const QColor &color);
    void populateUnderlineStyleComboBox();

private:
    FormatDescriptions m_descriptions;
    ColorScheme m_scheme;
    int m_curItem;
    Ui::ColorSchemeEdit *m_ui;
    FormatsModel *m_formatsModel;
    bool m_readOnly;
};


} // namespace Internal
} // namespace TextEditor
