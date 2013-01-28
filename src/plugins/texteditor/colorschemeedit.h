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

#ifndef COLORSCHEMEEDIT_H
#define COLORSCHEMEEDIT_H

#include "colorscheme.h"
#include "fontsettingspage.h"

#include <QDialog>

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
    void eraseForeColor();
    void checkCheckBoxes();

private:
    void updateControls();
    void setItemListBackground(const QColor &color);

    FormatDescriptions m_descriptions;
    ColorScheme m_scheme;
    int m_curItem;
    Ui::ColorSchemeEdit *m_ui;
    FormatsModel *m_formatsModel;
    bool m_readOnly;
};


} // namespace Internal
} // namespace TextEditor

#endif // COLORSCHEMEEDIT_H
