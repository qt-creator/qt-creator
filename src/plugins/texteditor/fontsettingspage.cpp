// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fontsettingspage.h"

#include "colorschemeedit.h"
#include "fontsettings.h"
#include "texteditorsettings.h"
#include "texteditortr.h"

#include <coreplugin/icore.h>

#include <utils/filepath.h>
#include <utils/fileutils.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/theme/theme.h>
#include <utils/utilsicons.h>

#include <QAbstractItemModel>
#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QFileDialog>
#include <QFontComboBox>
#include <QFontDatabase>
#include <QGroupBox>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPalette>
#include <QPointer>
#include <QPushButton>
#include <QSpacerItem>
#include <QSpinBox>
#include <QTimer>

using namespace TextEditor::Internal;
using namespace Utils;

namespace TextEditor {
namespace Internal {

struct ColorSchemeEntry
{
    ColorSchemeEntry(const FilePath &filePath, bool readOnly) :
        filePath(filePath),
        name(ColorScheme::readNameOfScheme(filePath)),
        readOnly(readOnly)
    { }

    FilePath filePath;
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
public:
    FontSettingsPageWidget(FontSettingsPage *q, const FormatDescriptions &fd, FontSettings *fontSettings)
        : q(q),
          m_value(*fontSettings),
          m_descriptions(fd)
    {
        m_lastValue = m_value;

        m_antialias = new QCheckBox(Tr::tr("Antialias"));
        m_antialias->setChecked(m_value.antialias());

        m_zoomSpinBox = new QSpinBox;
        m_zoomSpinBox->setSuffix(Tr::tr("%"));
        m_zoomSpinBox->setRange(10, 3000);
        m_zoomSpinBox->setSingleStep(10);
        m_zoomSpinBox->setValue(m_value.fontZoom());

        m_lineSpacingSpinBox = new QSpinBox;
        m_lineSpacingSpinBox->setSuffix(Tr::tr("%"));
        m_lineSpacingSpinBox->setRange(50, 3000);
        m_lineSpacingSpinBox->setValue(m_value.relativeLineSpacing());

        m_lineSpacingWarningLabel = new QLabel;
        m_lineSpacingWarningLabel->setPixmap(Utils::Icons::WARNING.pixmap());
        m_lineSpacingWarningLabel->setToolTip(Tr::tr("A line spacing value other than 100% disables "
                                                 "text wrapping.\nA value less than 100% can result "
                                                 "in overlapping and misaligned graphics."));
        m_lineSpacingWarningLabel->setVisible(m_value.relativeLineSpacing() != 100);

        m_fontComboBox = new QFontComboBox;
        m_fontComboBox->setCurrentFont(m_value.family());

        m_sizeComboBox = new QComboBox;
        m_sizeComboBox->setEditable(true);
        auto sizeValidator = new QIntValidator(m_sizeComboBox);
        sizeValidator->setBottom(0);
        m_sizeComboBox->setValidator(sizeValidator);

        m_copyButton = new QPushButton(Tr::tr("Copy..."));

        m_deleteButton = new QPushButton(Tr::tr("Delete"));
        m_deleteButton->setEnabled(false);

        auto importButton = new QPushButton(Tr::tr("Import"));
        auto exportButton = new QPushButton(Tr::tr("Export"));

        m_schemeComboBox = new QComboBox;
        m_schemeComboBox->setModel(&m_schemeListModel);
        m_schemeComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        m_schemeEdit = new ColorSchemeEdit;
        m_schemeEdit->setFormatDescriptions(fd);
        m_schemeEdit->setBaseFont(m_value.font());
        m_schemeEdit->setColorScheme(m_value.colorScheme());

        using namespace Layouting;

        Column {
            Group {
                title(Tr::tr("Font")),
                Column {
                    Row {
                        Tr::tr("Family:"), m_fontComboBox, Space(20),
                        Tr::tr("Size:"), m_sizeComboBox, Space(20),
                        Tr::tr("Zoom:"), m_zoomSpinBox, Space(20),
                        Tr::tr("Line spacing:"), m_lineSpacingSpinBox, m_lineSpacingWarningLabel, st
                    },
                    m_antialias
                }
            },
            Group {
                title(Tr::tr("Color Scheme for Theme \"%1\"")
                    .arg(Utils::creatorTheme()->displayName())),
                Column {
                    Row { m_schemeComboBox, m_copyButton, m_deleteButton, importButton, exportButton },
                    m_schemeEdit
                }
            }

        }.attachTo(this);

        connect(m_fontComboBox, &QFontComboBox::currentFontChanged,
                this, &FontSettingsPageWidget::fontSelected);
        connect(m_sizeComboBox, &QComboBox::currentIndexChanged,
                this, &FontSettingsPageWidget::fontSizeSelected);
        connect(m_zoomSpinBox, &QSpinBox::valueChanged,
                this, &FontSettingsPageWidget::fontZoomChanged);
        connect(m_antialias, &QCheckBox::toggled,
                this, &FontSettingsPageWidget::antialiasChanged);
        connect(m_lineSpacingSpinBox, &QSpinBox::valueChanged,
                this, &FontSettingsPageWidget::lineSpacingChanged);
        connect(m_schemeComboBox, &QComboBox::currentIndexChanged,
                this, &FontSettingsPageWidget::colorSchemeSelected);
        connect(m_copyButton, &QPushButton::clicked,
                this, &FontSettingsPageWidget::openCopyColorSchemeDialog);
        connect(m_schemeEdit, &ColorSchemeEdit::copyScheme,
                this, &FontSettingsPageWidget::openCopyColorSchemeDialog);
        connect(m_deleteButton, &QPushButton::clicked,
                this, &FontSettingsPageWidget::confirmDeleteColorScheme);
        connect(importButton, &QPushButton::clicked,
                this, &FontSettingsPageWidget::importScheme);
        connect(exportButton, &QPushButton::clicked,
                this, &FontSettingsPageWidget::exportScheme);
        connect(TextEditorSettings::instance(), &TextEditorSettings::fontSettingsChanged,
                this, &FontSettingsPageWidget::updateFontZoom);

        updatePointSizes();
        refreshColorSchemeList();
    }

    void apply() final;
    void finish() final;

    void saveSettings();
    void fontSelected(const QFont &font);
    void fontSizeSelected(int index);
    void fontZoomChanged();
    void lineSpacingChanged(const int &value);
    void antialiasChanged();
    void colorSchemeSelected(int index);
    void openCopyColorSchemeDialog();
    void copyColorScheme(const QString &name);
    void confirmDeleteColorScheme();
    void importScheme();
    void exportScheme();
    void deleteColorScheme();

    void maybeSaveColorScheme();
    void updatePointSizes();
    void updateFontZoom(const FontSettings &fontSettings);
    QList<int> pointSizesForSelectedFont() const;
    void refreshColorSchemeList();

    FontSettingsPage *q;
    bool m_refreshingSchemeList = false;
    FontSettings &m_value;
    FontSettings m_lastValue;
    SchemeListModel m_schemeListModel;
    FormatDescriptions m_descriptions;

    QCheckBox *m_antialias;
    QSpinBox *m_zoomSpinBox;
    QSpinBox *m_lineSpacingSpinBox;
    QLabel *m_lineSpacingWarningLabel;
    QFontComboBox *m_fontComboBox;
    QComboBox *m_sizeComboBox;
    QComboBox *m_schemeComboBox;
    ColorSchemeEdit *m_schemeEdit;
    QPushButton *m_deleteButton;
    QPushButton *m_copyButton;
};

} // namespace Internal

static FilePath customStylesPath()
{
    return Core::ICore::userResourcePath("styles");
}

static FilePath createColorSchemeFileName(const QString &pattern)
{
    const FilePath stylesPath = customStylesPath();

    // Find an available file name
    int i = 1;
    FilePath filePath;
    do {
        filePath = stylesPath.pathAppended(pattern.arg((i == 1) ? QString() : QString::number(i)));
        ++i;
    } while (filePath.exists());

    // Create the base directory when it doesn't exist
    if (!stylesPath.exists() && !stylesPath.createDir()) {
        qWarning() << "Failed to create color scheme directory:" << stylesPath;
        return {};
    }

    return filePath;
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
    if (id == C_TEXT) {
        return Qt::black;
    } else if (id == C_LINE_NUMBER) {
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
    } else if (id == C_SEARCH_RESULT_ALT1) {
        return QColor(0x00, 0x00, 0x33);
    } else if (id == C_SEARCH_RESULT_ALT2) {
        return QColor(0x33, 0x00, 0x00);
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
    } else if (id == C_SEARCH_RESULT_ALT1) {
        return QColor(0xb6, 0xcc, 0xff);
    } else if (id == C_SEARCH_RESULT_ALT2) {
        return QColor(0xff, 0xb6, 0xcc);
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
        const qreal ratio = ((palette.color(QPalette::Text).value() < 128) !=
                (palette.color(QPalette::HighlightedText).value() < 128)) ? smallRatio : largeRatio;

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

namespace Internal {

void FontSettingsPageWidget::fontSelected(const QFont &font)
{
    m_value.setFamily(font.family());
    m_schemeEdit->setBaseFont(font);
    updatePointSizes();
}

void FontSettingsPageWidget::updatePointSizes()
{
    // Update point sizes
    const int oldSize = m_value.fontSize();
    m_sizeComboBox->clear();
    const QList<int> sizeLst = pointSizesForSelectedFont();
    int idx = -1;
    int i = 0;
    for (; i < sizeLst.count(); ++i) {
        if (idx == -1 && sizeLst.at(i) >= oldSize) {
            idx = i;
            if (sizeLst.at(i) != oldSize)
                m_sizeComboBox->addItem(QString::number(oldSize));
        }
        m_sizeComboBox->addItem(QString::number(sizeLst.at(i)));
    }
    if (idx != -1)
        m_sizeComboBox->setCurrentIndex(idx);
}

void FontSettingsPageWidget::updateFontZoom(const FontSettings &fontSettings)
{
    m_zoomSpinBox->setValue(fontSettings.fontZoom());
}

QList<int> FontSettingsPageWidget::pointSizesForSelectedFont() const
{
    QFontDatabase db;
    const QString familyName = m_fontComboBox->currentFont().family();
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
    const QString sizeString = m_sizeComboBox->itemText(index);
    bool ok = true;
    const int size = sizeString.toInt(&ok);
    if (ok) {
        m_value.setFontSize(size);
        m_schemeEdit->setBaseFont(m_value.font());
    }
}

void FontSettingsPageWidget::fontZoomChanged()
{
    m_value.setFontZoom(m_zoomSpinBox->value());
}

void FontSettingsPageWidget::antialiasChanged()
{
    m_value.setAntialias(m_antialias->isChecked());
    m_schemeEdit->setBaseFont(m_value.font());
}

void FontSettingsPageWidget::lineSpacingChanged(const int &value)
{
    m_value.setRelativeLineSpacing(value);
    m_lineSpacingWarningLabel->setVisible(value != 100);
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
        m_value.loadColorScheme(entry.filePath, m_descriptions);
        m_schemeEdit->setColorScheme(m_value.colorScheme());
    }
    m_copyButton->setEnabled(index != -1);
    m_deleteButton->setEnabled(!readOnly);
    m_schemeEdit->setReadOnly(readOnly);
}

void FontSettingsPageWidget::openCopyColorSchemeDialog()
{
    QInputDialog *dialog = new QInputDialog(m_copyButton->window());
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setInputMode(QInputDialog::TextInput);
    dialog->setWindowTitle(Tr::tr("Copy Color Scheme"));
    dialog->setLabelText(Tr::tr("Color scheme name:"));
    dialog->setTextValue(Tr::tr("%1 (copy)").arg(m_value.colorScheme().displayName()));

    connect(dialog, &QInputDialog::textValueSelected, this, &FontSettingsPageWidget::copyColorScheme);
    dialog->open();
}

void FontSettingsPageWidget::copyColorScheme(const QString &name)
{
    int index = m_schemeComboBox->currentIndex();
    if (index == -1)
        return;

    const ColorSchemeEntry &entry = m_schemeListModel.colorSchemeAt(index);

    QString baseFileName = entry.filePath.completeBaseName();
    baseFileName += QLatin1String("_copy%1.xml");
    FilePath filePath = createColorSchemeFileName(baseFileName);

    if (!filePath.isEmpty()) {
        // Ask about saving any existing modifications
        maybeSaveColorScheme();

        // Make sure we're copying the current version
        m_value.setColorScheme(m_schemeEdit->colorScheme());

        ColorScheme scheme = m_value.colorScheme();
        scheme.setDisplayName(name);
        if (scheme.save(filePath, Core::ICore::dialogParent()))
            m_value.setColorSchemeFileName(filePath);

        refreshColorSchemeList();
    }
}

void FontSettingsPageWidget::confirmDeleteColorScheme()
{
    const int index = m_schemeComboBox->currentIndex();
    if (index == -1)
        return;

    const ColorSchemeEntry &entry = m_schemeListModel.colorSchemeAt(index);
    if (entry.readOnly)
        return;

    QMessageBox *messageBox = new QMessageBox(QMessageBox::Warning,
                                              Tr::tr("Delete Color Scheme"),
                                              Tr::tr("Are you sure you want to delete this color scheme permanently?"),
                                              QMessageBox::Discard | QMessageBox::Cancel,
                                              m_deleteButton->window());

    // Change the text and role of the discard button
    auto deleteButton = static_cast<QPushButton*>(messageBox->button(QMessageBox::Discard));
    deleteButton->setText(Tr::tr("Delete"));
    messageBox->addButton(deleteButton, QMessageBox::AcceptRole);
    messageBox->setDefaultButton(deleteButton);

    connect(messageBox, &QDialog::accepted, this, &FontSettingsPageWidget::deleteColorScheme);
    messageBox->setAttribute(Qt::WA_DeleteOnClose);
    messageBox->open();
}

void FontSettingsPageWidget::deleteColorScheme()
{
    const int index = m_schemeComboBox->currentIndex();
    QTC_ASSERT(index != -1, return);

    const ColorSchemeEntry &entry = m_schemeListModel.colorSchemeAt(index);
    QTC_ASSERT(!entry.readOnly, return);

    if (entry.filePath.removeFile())
        m_schemeListModel.removeColorScheme(index);
}

void FontSettingsPageWidget::importScheme()
{
    const FilePath importedFile
        = Utils::FileUtils::getOpenFilePath(this,
                                            Tr::tr("Import Color Scheme"),
                                            {},
                                            Tr::tr("Color scheme (*.xml);;All files (*)"));

    if (importedFile.isEmpty())
        return;

    // Ask about saving any existing modifications
    maybeSaveColorScheme();

    QInputDialog *dialog = new QInputDialog(m_copyButton->window());
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setInputMode(QInputDialog::TextInput);
    dialog->setWindowTitle(Tr::tr("Import Color Scheme"));
    dialog->setLabelText(Tr::tr("Color scheme name:"));
    dialog->setTextValue(importedFile.baseName());

    connect(dialog,
            &QInputDialog::textValueSelected,
            this,
            [this, importedFile](const QString &name) {
                const Utils::FilePath saveFileName = createColorSchemeFileName(
                    importedFile.baseName() + "%1." + importedFile.suffix());

                ColorScheme scheme;
                if (scheme.load(importedFile)) {
                    scheme.setDisplayName(name);
                    scheme.save(saveFileName, Core::ICore::dialogParent());
                    m_value.loadColorScheme(saveFileName, m_descriptions);
                } else {
                    qWarning() << "Failed to import color scheme:" << importedFile;
                }

                refreshColorSchemeList();
            });

    dialog->open();
}

void FontSettingsPageWidget::exportScheme()
{
    int index = m_schemeComboBox->currentIndex();
    if (index == -1)
        return;

    const ColorSchemeEntry &entry = m_schemeListModel.colorSchemeAt(index);

    const FilePath filePath
        = Utils::FileUtils::getSaveFilePath(this,
                                            Tr::tr("Export Color Scheme"),
                                            entry.filePath,
                                            Tr::tr("Color scheme (*.xml);;All files (*)"));

    if (!filePath.isEmpty())
        m_value.colorScheme().save(filePath, Core::ICore::dialogParent());
}

void FontSettingsPageWidget::maybeSaveColorScheme()
{
    if (m_value.colorScheme() == m_schemeEdit->colorScheme())
        return;

    QMessageBox
        messageBox(QMessageBox::Warning,
                   Tr::tr("Color Scheme Changed"),
                   Tr::tr("The color scheme \"%1\" was modified, do you want to save the changes?")
                       .arg(m_schemeEdit->colorScheme().displayName()),
                   QMessageBox::Discard | QMessageBox::Save,
                   m_schemeComboBox->window());

    // Change the text of the discard button
    auto discardButton = static_cast<QPushButton*>(messageBox.button(QMessageBox::Discard));
    discardButton->setText(Tr::tr("Discard"));
    messageBox.addButton(discardButton, QMessageBox::DestructiveRole);
    messageBox.setDefaultButton(QMessageBox::Save);

    if (messageBox.exec() == QMessageBox::Save) {
        const ColorScheme &scheme = m_schemeEdit->colorScheme();
        scheme.save(m_value.colorSchemeFileName(), Core::ICore::dialogParent());
    }
}

void FontSettingsPageWidget::refreshColorSchemeList()
{
    QList<ColorSchemeEntry> colorSchemes;

    const FilePath styleDir = Core::ICore::resourcePath("styles");

    FilePaths schemeList = styleDir.dirEntries(FileFilter({"*.xml"}, QDir::Files));
    const FilePath defaultScheme = FontSettings::defaultSchemeFileName();

    if (schemeList.removeAll(defaultScheme))
        schemeList.prepend(defaultScheme);

    int selected = 0;

    for (const FilePath &file : std::as_const(schemeList)) {
        if (m_value.colorSchemeFileName().fileName() == file.fileName())
            selected = colorSchemes.size();
        colorSchemes.append(ColorSchemeEntry(file, true));
    }

    if (colorSchemes.isEmpty())
        qWarning() << "Warning: no color schemes found in path:" << styleDir.toUserOutput();

    const FilePaths files = customStylesPath().dirEntries(FileFilter({"*.xml"}, QDir::Files));
    for (const FilePath &file : files) {
        if (m_value.colorSchemeFileName().fileName() == file.fileName())
            selected = colorSchemes.size();
        colorSchemes.append(ColorSchemeEntry(file, false));
    }

    m_refreshingSchemeList = true;
    m_schemeListModel.setColorSchemes(colorSchemes);
    m_schemeComboBox->setCurrentIndex(selected);
    m_refreshingSchemeList = false;
}

void FontSettingsPageWidget::apply()
{
    if (m_value.colorScheme() != m_schemeEdit->colorScheme()) {
        // Update the scheme and save it under the name it already has
        m_value.setColorScheme(m_schemeEdit->colorScheme());
        const ColorScheme &scheme = m_value.colorScheme();
        scheme.save(m_value.colorSchemeFileName(), Core::ICore::dialogParent());
    }

    bool ok;
    int fontSize = m_sizeComboBox->currentText().toInt(&ok);
    if (ok && m_value.fontSize() != fontSize) {
        m_value.setFontSize(fontSize);
        m_schemeEdit->setBaseFont(m_value.font());
    }

    int index = m_schemeComboBox->currentIndex();
    if (index != -1) {
        const ColorSchemeEntry &entry = m_schemeListModel.colorSchemeAt(index);
        if (entry.filePath != m_value.colorSchemeFileName())
            m_value.loadColorScheme(entry.filePath, m_descriptions);
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
    QtcSettings *settings = Core::ICore::settings();
    if (settings)
       fontSettings->fromSettings(fd, settings);

    if (fontSettings->colorSchemeFileName().isEmpty())
       fontSettings->loadColorScheme(FontSettings::defaultSchemeFileName(), fd);

    setId(Constants::TEXT_EDITOR_FONT_SETTINGS);
    setDisplayName(Tr::tr("Font && Colors"));
    setCategory(TextEditor::Constants::TEXT_EDITOR_SETTINGS_CATEGORY);
    setDisplayCategory(Tr::tr("Text Editor"));
    setCategoryIconPath(TextEditor::Constants::TEXT_EDITOR_SETTINGS_CATEGORY_ICON_PATH);
    setWidgetCreator([this, fontSettings, fd] { return new FontSettingsPageWidget(this, fd, fontSettings); });
}

} // TextEditor
