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

#include "fontsettingspage.h"

#include "fontsettings.h"
#include "texteditorsettings.h"
#include "ui_fontsettingspage.h"

#include <coreplugin/icore.h>
#include <utils/fileutils.h>
#include <utils/stringutils.h>
#include <utils/qtcassert.h>
#include <utils/theme/theme.h>

#include <QFileDialog>
#include <QFontDatabase>
#include <QInputDialog>
#include <QMessageBox>
#include <QPalette>
#include <QPointer>
#include <QSettings>
#include <QTimer>
#include <QDebug>

using namespace TextEditor::Internal;

namespace TextEditor {
namespace Internal {

struct ColorSchemeEntry
{
    ColorSchemeEntry(const QString &fileName,
                     bool readOnly):
        fileName(fileName),
        name(ColorScheme::readNameOfScheme(fileName)),
        readOnly(readOnly)
    { }

    QString fileName;
    QString name;
    QString id;
    bool readOnly;
};


class SchemeListModel : public QAbstractListModel
{
public:
    SchemeListModel(QObject *parent = nullptr):
        QAbstractListModel(parent)
    {
    }

    int rowCount(const QModelIndex &parent) const override
    { return parent.isValid() ? 0 : m_colorSchemes.size(); }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (role == Qt::DisplayRole)
            return m_colorSchemes.at(index.row()).name;

        return QVariant();
    }

    void removeColorScheme(int index)
    {
        beginRemoveRows(QModelIndex(), index, index);
        m_colorSchemes.removeAt(index);
        endRemoveRows();
    }

    void setColorSchemes(const QList<ColorSchemeEntry> &colorSchemes)
    {
        beginResetModel();
        m_colorSchemes = colorSchemes;
        endResetModel();
    }

    const ColorSchemeEntry &colorSchemeAt(int index) const
    { return m_colorSchemes.at(index); }

private:
    QList<ColorSchemeEntry> m_colorSchemes;
};

class FontSettingsPageWidget : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(TextEditor::FontSettingsPageWidget)

public:
    FontSettingsPageWidget(FontSettingsPage *q, const FormatDescriptions &fd, FontSettings *fontSettings)
        : q(q),
          m_value(*fontSettings),
          m_descriptions(fd)
    {
        m_lastValue = m_value;

        m_ui.setupUi(this);
        m_ui.colorSchemeGroupBox->setTitle(
                    tr("Color Scheme for Theme \"%1\"")
                    .arg(Utils::creatorTheme()->displayName()));
        m_ui.schemeComboBox->setModel(&m_schemeListModel);

        m_ui.fontComboBox->setCurrentFont(m_value.family());

        m_ui.antialias->setChecked(m_value.antialias());
        m_ui.zoomSpinBox->setValue(m_value.fontZoom());

        m_ui.schemeEdit->setFormatDescriptions(fd);
        m_ui.schemeEdit->setBaseFont(m_value.font());
        m_ui.schemeEdit->setColorScheme(m_value.colorScheme());

        auto sizeValidator = new QIntValidator(m_ui.sizeComboBox);
        sizeValidator->setBottom(0);
        m_ui.sizeComboBox->setValidator(sizeValidator);

        connect(m_ui.fontComboBox, &QFontComboBox::currentFontChanged,
                this, &FontSettingsPageWidget::fontSelected);
        connect(m_ui.sizeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &FontSettingsPageWidget::fontSizeSelected);
        connect(m_ui.zoomSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                this, &FontSettingsPageWidget::fontZoomChanged);
        connect(m_ui.antialias, &QCheckBox::toggled,
                this, &FontSettingsPageWidget::antialiasChanged);
        connect(m_ui.schemeComboBox,
                QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &FontSettingsPageWidget::colorSchemeSelected);
        connect(m_ui.copyButton, &QPushButton::clicked,
                this, &FontSettingsPageWidget::openCopyColorSchemeDialog);
        connect(m_ui.schemeEdit, &ColorSchemeEdit::copyScheme,
                this, &FontSettingsPageWidget::openCopyColorSchemeDialog);
        connect(m_ui.deleteButton, &QPushButton::clicked,
                this, &FontSettingsPageWidget::confirmDeleteColorScheme);

        updatePointSizes();
        refreshColorSchemeList();
    }

    void apply() final;
    void finish() final;

    void saveSettings();
    void fontSelected(const QFont &font);
    void fontSizeSelected(int index);
    void fontZoomChanged();
    void antialiasChanged();
    void colorSchemeSelected(int index);
    void openCopyColorSchemeDialog();
    void copyColorScheme(const QString &name);
    void confirmDeleteColorScheme();
    void deleteColorScheme();

    void maybeSaveColorScheme();
    void updatePointSizes();
    QList<int> pointSizesForSelectedFont() const;
    void refreshColorSchemeList();

    FontSettingsPage *q;
    Ui::FontSettingsPage m_ui;
    bool m_refreshingSchemeList = false;
    FontSettings &m_value;
    FontSettings m_lastValue;
    SchemeListModel m_schemeListModel;
    FormatDescriptions m_descriptions;
};

} // namespace Internal

static QString customStylesPath()
{
    QString path = Core::ICore::userResourcePath();
    path.append(QLatin1String("/styles/"));
    return path;
}

static QString createColorSchemeFileName(const QString &pattern)
{
    const QString stylesPath = customStylesPath();
    QString baseFileName = stylesPath;
    baseFileName += pattern;

    // Find an available file name
    int i = 1;
    QString fileName;
    do {
        fileName = baseFileName.arg((i == 1) ? QString() : QString::number(i));
        ++i;
    } while (QFile::exists(fileName));

    // Create the base directory when it doesn't exist
    if (!QFile::exists(stylesPath) && !QDir().mkpath(stylesPath)) {
        qWarning() << "Failed to create color scheme directory:" << stylesPath;
        return QString();
    }

    return fileName;
}

// ------- FormatDescription
FormatDescription::FormatDescription(TextStyle id,
                                     const QString &displayName,
                                     const QString &tooltipText,
                                     const QColor &foreground,
                                     FormatDescription::ShowControls showControls)
    : m_id(id),
      m_displayName(displayName),
      m_tooltipText(tooltipText),
      m_showControls(showControls)
{
    m_format.setForeground(foreground);
    m_format.setBackground(defaultBackground(id));
}

FormatDescription::FormatDescription(TextStyle id,
                                     const QString &displayName,
                                     const QString &tooltipText,
                                     const Format &format,
                                     FormatDescription::ShowControls showControls)
    : m_id(id),
      m_format(format),
      m_displayName(displayName),
      m_tooltipText(tooltipText),
      m_showControls(showControls)
{
}

FormatDescription::FormatDescription(TextStyle id,
                                     const QString &displayName,
                                     const QString &tooltipText,
                                     const QColor &underlineColor,
                                     const QTextCharFormat::UnderlineStyle underlineStyle,
                                     FormatDescription::ShowControls showControls)
    : m_id(id),
      m_displayName(displayName),
      m_tooltipText(tooltipText),
      m_showControls(showControls)
{
    m_format.setForeground(defaultForeground(id));
    m_format.setBackground(defaultBackground(id));
    m_format.setUnderlineColor(underlineColor);
    m_format.setUnderlineStyle(underlineStyle);
}

FormatDescription::FormatDescription(TextStyle id,
                                     const QString &displayName,
                                     const QString &tooltipText,
                                     FormatDescription::ShowControls showControls)
    : m_id(id),
      m_displayName(displayName),
      m_tooltipText(tooltipText),
      m_showControls(showControls)
{
    m_format.setForeground(defaultForeground(id));
    m_format.setBackground(defaultBackground(id));
}

QColor FormatDescription::defaultForeground(TextStyle id)
{
    if (id == C_LINE_NUMBER) {
        const QPalette palette = Utils::Theme::initialPalette();
        const QColor bg = palette.window().color();
        if (bg.value() < 128)
            return palette.windowText().color();
        else
            return palette.dark().color();
    } else if (id == C_CURRENT_LINE_NUMBER) {
        const QPalette palette = Utils::Theme::initialPalette();
        const QColor bg = palette.window().color();
        if (bg.value() < 128)
            return palette.windowText().color();
        else
            return QColor();
    } else if (id == C_PARENTHESES) {
        return QColor(Qt::red);
    } else if (id == C_AUTOCOMPLETE) {
        return QColor(Qt::darkBlue);
    }
    return QColor();
}

QColor FormatDescription::defaultBackground(TextStyle id)
{
    if (id == C_TEXT) {
        return Qt::white;
    } else if (id == C_LINE_NUMBER) {
        return Utils::Theme::initialPalette().window().color();
    } else if (id == C_SEARCH_RESULT) {
        return QColor(0xffef0b);
    } else if (id == C_PARENTHESES) {
        return QColor(0xb4, 0xee, 0xb4);
    } else if (id == C_PARENTHESES_MISMATCH) {
        return QColor(Qt::magenta);
    } else if (id == C_AUTOCOMPLETE) {
        return QColor(192, 192, 255);
    } else if (id == C_CURRENT_LINE || id == C_SEARCH_SCOPE) {
        const QPalette palette = Utils::Theme::initialPalette();
        const QColor &fg = palette.color(QPalette::Highlight);
        const QColor &bg = palette.color(QPalette::Base);

        qreal smallRatio;
        qreal largeRatio;
        if (id == C_CURRENT_LINE) {
            smallRatio = .3;
            largeRatio = .6;
        } else {
            smallRatio = .05;
            largeRatio = .4;
        }
        const qreal ratio = ((palette.color(QPalette::Text).value() < 128)
                             ^ (palette.color(QPalette::HighlightedText).value() < 128)) ? smallRatio : largeRatio;

        const QColor &col = QColor::fromRgbF(fg.redF() * ratio + bg.redF() * (1 - ratio),
                                             fg.greenF() * ratio + bg.greenF() * (1 - ratio),
                                             fg.blueF() * ratio + bg.blueF() * (1 - ratio));
        return col;
    } else if (id == C_SELECTION) {
        return Utils::Theme::initialPalette().color(QPalette::Highlight);
    } else if (id == C_OCCURRENCES) {
        return QColor(180, 180, 180);
    } else if (id == C_OCCURRENCES_RENAME) {
        return QColor(255, 100, 100);
    } else if (id == C_DISABLED_CODE) {
        return QColor(239, 239, 239);
    }
    return QColor(); // invalid color
}

bool FormatDescription::showControl(FormatDescription::ShowControls showControl) const
{
    return m_showControls & showControl;
}

void FontSettingsPageWidget::fontSelected(const QFont &font)
{
    m_value.setFamily(font.family());
    m_ui.schemeEdit->setBaseFont(font);
    updatePointSizes();
}

namespace Internal {

void FontSettingsPageWidget::updatePointSizes()
{
    // Update point sizes
    const int oldSize = m_value.fontSize();
    m_ui.sizeComboBox->clear();
    const QList<int> sizeLst = pointSizesForSelectedFont();
    int idx = -1;
    int i = 0;
    for (; i < sizeLst.count(); ++i) {
        if (idx == -1 && sizeLst.at(i) >= oldSize) {
            idx = i;
            if (sizeLst.at(i) != oldSize)
                m_ui.sizeComboBox->addItem(QString::number(oldSize));
        }
        m_ui.sizeComboBox->addItem(QString::number(sizeLst.at(i)));
    }
    if (idx != -1)
        m_ui.sizeComboBox->setCurrentIndex(idx);
}

QList<int> FontSettingsPageWidget::pointSizesForSelectedFont() const
{
    QFontDatabase db;
    const QString familyName = m_ui.fontComboBox->currentFont().family();
    QList<int> sizeLst = db.pointSizes(familyName);
    if (!sizeLst.isEmpty())
        return sizeLst;

    QStringList styles = db.styles(familyName);
    if (!styles.isEmpty())
        sizeLst = db.pointSizes(familyName, styles.first());
    if (sizeLst.isEmpty())
        sizeLst = QFontDatabase::standardSizes();

    return sizeLst;
}

void FontSettingsPageWidget::fontSizeSelected(int index)
{
    const QString sizeString = m_ui.sizeComboBox->itemText(index);
    bool ok = true;
    const int size = sizeString.toInt(&ok);
    if (ok) {
        m_value.setFontSize(size);
        m_ui.schemeEdit->setBaseFont(m_value.font());
    }
}

void FontSettingsPageWidget::fontZoomChanged()
{
    m_value.setFontZoom(m_ui.zoomSpinBox->value());
}

void FontSettingsPageWidget::antialiasChanged()
{
    m_value.setAntialias(m_ui.antialias->isChecked());
    m_ui.schemeEdit->setBaseFont(m_value.font());
}

void FontSettingsPageWidget::colorSchemeSelected(int index)
{
    bool readOnly = true;
    if (index != -1) {
        // Check whether we're switching away from a changed color scheme
        if (!m_refreshingSchemeList)
            maybeSaveColorScheme();

        const ColorSchemeEntry &entry = m_schemeListModel.colorSchemeAt(index);
        readOnly = entry.readOnly;
        m_value.loadColorScheme(entry.fileName, m_descriptions);
        m_ui.schemeEdit->setColorScheme(m_value.colorScheme());
    }
    m_ui.copyButton->setEnabled(index != -1);
    m_ui.deleteButton->setEnabled(!readOnly);
    m_ui.schemeEdit->setReadOnly(readOnly);
}

void FontSettingsPageWidget::openCopyColorSchemeDialog()
{
    QInputDialog *dialog = new QInputDialog(m_ui.copyButton->window());
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setInputMode(QInputDialog::TextInput);
    dialog->setWindowTitle(tr("Copy Color Scheme"));
    dialog->setLabelText(tr("Color scheme name:"));
    dialog->setTextValue(tr("%1 (copy)").arg(m_value.colorScheme().displayName()));

    connect(dialog, &QInputDialog::textValueSelected, this, &FontSettingsPageWidget::copyColorScheme);
    dialog->open();
}

void FontSettingsPageWidget::copyColorScheme(const QString &name)
{
    int index = m_ui.schemeComboBox->currentIndex();
    if (index == -1)
        return;

    const ColorSchemeEntry &entry = m_schemeListModel.colorSchemeAt(index);

    QString baseFileName = QFileInfo(entry.fileName).completeBaseName();
    baseFileName += QLatin1String("_copy%1.xml");
    QString fileName = createColorSchemeFileName(baseFileName);

    if (!fileName.isEmpty()) {
        // Ask about saving any existing modifactions
        maybeSaveColorScheme();

        // Make sure we're copying the current version
        m_value.setColorScheme(m_ui.schemeEdit->colorScheme());

        ColorScheme scheme = m_value.colorScheme();
        scheme.setDisplayName(name);
        if (scheme.save(fileName, Core::ICore::dialogParent()))
            m_value.setColorSchemeFileName(fileName);

        refreshColorSchemeList();
    }
}

void FontSettingsPageWidget::confirmDeleteColorScheme()
{
    const int index = m_ui.schemeComboBox->currentIndex();
    if (index == -1)
        return;

    const ColorSchemeEntry &entry = m_schemeListModel.colorSchemeAt(index);
    if (entry.readOnly)
        return;

    QMessageBox *messageBox = new QMessageBox(QMessageBox::Warning,
                                              tr("Delete Color Scheme"),
                                              tr("Are you sure you want to delete this color scheme permanently?"),
                                              QMessageBox::Discard | QMessageBox::Cancel,
                                              m_ui.deleteButton->window());

    // Change the text and role of the discard button
    auto deleteButton = static_cast<QPushButton*>(messageBox->button(QMessageBox::Discard));
    deleteButton->setText(tr("Delete"));
    messageBox->addButton(deleteButton, QMessageBox::AcceptRole);
    messageBox->setDefaultButton(deleteButton);

    connect(deleteButton, &QAbstractButton::clicked, messageBox, &QDialog::accept);
    connect(messageBox, &QDialog::accepted, this, &FontSettingsPageWidget::deleteColorScheme);
    messageBox->setAttribute(Qt::WA_DeleteOnClose);
    messageBox->open();
}

void FontSettingsPageWidget::deleteColorScheme()
{
    const int index = m_ui.schemeComboBox->currentIndex();
    QTC_ASSERT(index != -1, return);

    const ColorSchemeEntry &entry = m_schemeListModel.colorSchemeAt(index);
    QTC_ASSERT(!entry.readOnly, return);

    if (QFile::remove(entry.fileName))
        m_schemeListModel.removeColorScheme(index);
}

void FontSettingsPageWidget::maybeSaveColorScheme()
{
    if (m_value.colorScheme() == m_ui.schemeEdit->colorScheme())
        return;

    QMessageBox messageBox(QMessageBox::Warning,
                                              tr("Color Scheme Changed"),
                                              tr("The color scheme \"%1\" was modified, do you want to save the changes?")
                                                  .arg(m_ui.schemeEdit->colorScheme().displayName()),
                                              QMessageBox::Discard | QMessageBox::Save,
                                              m_ui.schemeComboBox->window());

    // Change the text of the discard button
    auto discardButton = static_cast<QPushButton*>(messageBox.button(QMessageBox::Discard));
    discardButton->setText(tr("Discard"));
    messageBox.addButton(discardButton, QMessageBox::DestructiveRole);
    messageBox.setDefaultButton(QMessageBox::Save);

    if (messageBox.exec() == QMessageBox::Save) {
        const ColorScheme &scheme = m_ui.schemeEdit->colorScheme();
        scheme.save(m_value.colorSchemeFileName(), Core::ICore::dialogParent());
    }
}

void FontSettingsPageWidget::refreshColorSchemeList()
{
    QList<ColorSchemeEntry> colorSchemes;

    QString resourcePath = Core::ICore::resourcePath();
    QDir styleDir(resourcePath + QLatin1String("/styles"));
    styleDir.setNameFilters(QStringList() << QLatin1String("*.xml"));
    styleDir.setFilter(QDir::Files);

    int selected = 0;

    QStringList schemeList = styleDir.entryList();
    QString defaultScheme = Utils::FilePath::fromString(FontSettings::defaultSchemeFileName()).fileName();
    if (schemeList.removeAll(defaultScheme))
        schemeList.prepend(defaultScheme);
    foreach (const QString &file, schemeList) {
        const QString fileName = styleDir.absoluteFilePath(file);
        if (m_value.colorSchemeFileName() == fileName)
            selected = colorSchemes.size();
        colorSchemes.append(ColorSchemeEntry(fileName, true));
    }

    if (colorSchemes.isEmpty())
        qWarning() << "Warning: no color schemes found in path:" << styleDir.path();

    styleDir.setPath(customStylesPath());

    foreach (const QString &file, styleDir.entryList()) {
        const QString fileName = styleDir.absoluteFilePath(file);
        if (m_value.colorSchemeFileName() == fileName)
            selected = colorSchemes.size();
        colorSchemes.append(ColorSchemeEntry(fileName, false));
    }

    m_refreshingSchemeList = true;
    m_schemeListModel.setColorSchemes(colorSchemes);
    m_ui.schemeComboBox->setCurrentIndex(selected);
    m_refreshingSchemeList = false;
}

void FontSettingsPageWidget::apply()
{
    if (m_value.colorScheme() != m_ui.schemeEdit->colorScheme()) {
        // Update the scheme and save it under the name it already has
        m_value.setColorScheme(m_ui.schemeEdit->colorScheme());
        const ColorScheme &scheme = m_value.colorScheme();
        scheme.save(m_value.colorSchemeFileName(), Core::ICore::dialogParent());
    }

    bool ok;
    int fontSize = m_ui.sizeComboBox->currentText().toInt(&ok);
    if (ok && m_value.fontSize() != fontSize) {
        m_value.setFontSize(fontSize);
        m_ui.schemeEdit->setBaseFont(m_value.font());
    }

    int index = m_ui.schemeComboBox->currentIndex();
    if (index != -1) {
        const ColorSchemeEntry &entry = m_schemeListModel.colorSchemeAt(index);
        if (entry.fileName != m_value.colorSchemeFileName())
            m_value.loadColorScheme(entry.fileName, m_descriptions);
    }

    saveSettings();
}

void FontSettingsPageWidget::saveSettings()
{
    m_lastValue = m_value;
    m_value.toSettings(Core::ICore::settings());
    emit TextEditorSettings::instance()->fontSettingsChanged(m_value);
}

void FontSettingsPageWidget::finish()
{
    // If changes were applied, these are equal. Otherwise restores last value.
    m_value = m_lastValue;
}

} // namespace Internal

// FontSettingsPage

FontSettingsPage::FontSettingsPage(FontSettings *fontSettings, const FormatDescriptions &fd)
{
    QSettings *settings = Core::ICore::settings();
    if (settings)
       fontSettings->fromSettings(fd, settings);

    if (fontSettings->colorSchemeFileName().isEmpty())
       fontSettings->loadColorScheme(FontSettings::defaultSchemeFileName(), fd);

    setId(Constants::TEXT_EDITOR_FONT_SETTINGS);
    setDisplayName(FontSettingsPageWidget::tr("Font && Colors"));
    setCategory(TextEditor::Constants::TEXT_EDITOR_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("TextEditor", "Text Editor"));
    setCategoryIconPath(TextEditor::Constants::TEXT_EDITOR_SETTINGS_CATEGORY_ICON_PATH);
    setWidgetCreator([this, fontSettings, fd] { return new FontSettingsPageWidget(this, fd, fontSettings); });
}

void FontSettingsPage::setFontZoom(int zoom)
{
    if (m_widget)
        static_cast<FontSettingsPageWidget *>(m_widget.data())->m_ui.zoomSpinBox->setValue(zoom);
}

} // TextEditor
