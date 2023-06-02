// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "colorscheme.h"
#include "fontsettingspage.h"

#include <QDialog>

QT_BEGIN_NAMESPACE
class QAbstractButton;
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QListView;
class QModelIndex;
class QScrollArea;
class QToolButton;
QT_END_NAMESPACE

namespace Utils { class QtColorButton; }

namespace TextEditor::Internal {

class FormatsModel;

/*!
  A widget for editing a color scheme. Used in the FontSettingsPage.
  */
class ColorSchemeEdit : public QWidget
{
    Q_OBJECT

public:
    ColorSchemeEdit(QWidget *parent = nullptr);
    ~ColorSchemeEdit() override;

    void setFormatDescriptions(const FormatDescriptions &descriptions);
    void setBaseFont(const QFont &font);
    void setReadOnly(bool readOnly);

    void setColorScheme(const ColorScheme &colorScheme);
    const ColorScheme &colorScheme() const;

signals:
    void copyScheme();

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
    int m_curItem = -1;
    FormatsModel *m_formatsModel;
    bool m_readOnly = false;
    QListView *m_itemList;
    QLabel *m_builtinSchemeLabel;
    QWidget *m_fontProperties;
    QLabel *m_foregroundLabel;
    Utils::QtColorButton *m_foregroundToolButton;
    QAbstractButton *m_eraseForegroundToolButton;
    QLabel *m_backgroundLabel;
    Utils::QtColorButton *m_backgroundToolButton;
    QAbstractButton *m_eraseBackgroundToolButton;
    QLabel *m_relativeForegroundHeadline;
    QLabel *m_foregroundLightnessLabel;
    QDoubleSpinBox *m_foregroundLightnessSpinBox;
    QLabel *m_foregroundSaturationLabel;
    QDoubleSpinBox *m_foregroundSaturationSpinBox;
    QLabel *m_relativeBackgroundHeadline;
    QLabel *m_backgroundSaturationLabel;
    QDoubleSpinBox *m_backgroundSaturationSpinBox;
    QLabel *m_backgroundLightnessLabel;
    QDoubleSpinBox *m_backgroundLightnessSpinBox;
    QLabel *m_fontHeadline;
    QCheckBox *m_boldCheckBox;
    QCheckBox *m_italicCheckBox;
    QLabel *m_underlineHeadline;
    QLabel *m_underlineLabel;
    Utils::QtColorButton *m_underlineColorToolButton;
    QAbstractButton *m_eraseUnderlineColorToolButton;
    QComboBox *m_underlineComboBox;

};

} // TextEditor::Internal
