// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fontsettingspage.h"

#include "colorschemeedit.h"
#include "fontsettings.h"
#include "texteditortr.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/aspects.h>
#include <utils/filepath.h>
#include <utils/fileutils.h>
#include <utils/guiutils.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/theme/theme.h>
#include <utils/utilsicons.h>

#include <QAbstractItemModel>
#include <QApplication>
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
#include <QPushButton>
#include <QSpacerItem>
#include <QSpinBox>
#include <QTimer>

#ifdef WITH_TESTS
#include <QScopeGuard>
#include <QTest>
#endif

#include <functional>
#include <optional>

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

    int rowCount(const QModelIndex &parent) const final
    {
        return parent.isValid() ? 0 : m_colorSchemes.size();
    }

    QVariant data(const QModelIndex &index, int role) const final
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

class FontSettingsAspect final : public BaseAspect
{
    Q_OBJECT

public:
    explicit FontSettingsAspect(const FormatDescriptions &fd, AspectContainer *container = nullptr)
        : BaseAspect(container)
        , m_value(globalFontSettings().data())
        , m_descriptions(fd)
    {
        m_lastValue = m_value;
        setAutoApply(false);
    }

    void addToLayoutImpl(Layouting::Layout &parent) final;

    void apply() final;
    void cancel() final;
    bool isDirty() const final;

    // For the widgets showing the aspect.
    const FontSettingsData &value() const { return m_value; }
    void changeValue(const std::function<void(FontSettingsData &)> &change);
    const FormatDescriptions &descriptions() const { return m_descriptions; }
    SchemeListModel *schemeListModel() { return &m_schemeListModel; }
    int selectedScheme() const { return m_selectedScheme; }
    bool isSelectedSchemeReadOnly() const;
    ColorScheme shownColorScheme() const { return m_editedScheme.value_or(m_value.colorScheme()); }
    void setEditedScheme(const ColorScheme &scheme);

    void refreshColorSchemeList();
    void selectColorScheme(int index, QWidget *dialogParent);
    void copyColorScheme(const QString &name, QWidget *dialogParent);
    void deleteColorScheme();
    void importScheme(const Utils::FilePath &importedFile, const QString &name);
    void exportScheme();
    void maybeSaveColorScheme(QWidget *dialogParent);

signals:
    void valueChanged(QPrivateSignal);

private:
    void saveSettings();

    bool m_refreshingSchemeList = false;
    FontSettingsData m_value;
    FontSettingsData m_lastValue;
    SchemeListModel m_schemeListModel;
    FormatDescriptions m_descriptions;
    int m_selectedScheme = -1;
    // Changes made in a color scheme editor that are not saved yet.
    std::optional<ColorScheme> m_editedScheme;
};

// The widgets of one layout of the aspect.
class FontSettingsWidget final : public QWidget
{
public:
    explicit FontSettingsWidget(FontSettingsAspect *aspect);

private:
    void showValue();
    void updatePointSizes();
    QList<int> pointSizesForSelectedFont() const;
    void openCopyColorSchemeDialog();
    void confirmDeleteColorScheme();
    void importScheme();

    FontSettingsAspect *m_aspect;
    QString m_pointSizesFamily;
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
        return QColor(0xed, 0xb0, 0xed);
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

void FontSettingsAspect::addToLayoutImpl(Layouting::Layout &parent)
{
    auto widget = new FontSettingsWidget(this);
    registerSubWidget(widget);
    parent.addItem(widget);
}

void FontSettingsAspect::changeValue(const std::function<void(FontSettingsData &)> &change)
{
    const FontSettingsData old = m_value;
    change(m_value);
    if (m_value != old)
        emit valueChanged(QPrivateSignal());
}

bool FontSettingsAspect::isSelectedSchemeReadOnly() const
{
    if (m_selectedScheme < 0 || m_selectedScheme >= m_schemeListModel.rowCount({}))
        return true;
    return m_schemeListModel.colorSchemeAt(m_selectedScheme).readOnly;
}

void FontSettingsAspect::setEditedScheme(const ColorScheme &scheme)
{
    if (scheme == shownColorScheme())
        return;
    m_editedScheme = scheme;
    emit valueChanged(QPrivateSignal());
}

void FontSettingsAspect::selectColorScheme(int index, QWidget *dialogParent)
{
    // Refreshing the list keeps the scheme that is loaded, with its pending changes.
    if (m_refreshingSchemeList && index != -1
        && m_schemeListModel.colorSchemeAt(index).filePath == m_value.colorSchemeFileName()) {
        m_selectedScheme = index;
        emit valueChanged(QPrivateSignal());
        return;
    }
    if (index != -1) {
        // Check whether we're switching away from a changed color scheme
        if (!m_refreshingSchemeList)
            maybeSaveColorScheme(dialogParent);

        const ColorSchemeEntry &entry = m_schemeListModel.colorSchemeAt(index);
        m_value.loadColorScheme(entry.filePath, m_descriptions);
        m_editedScheme.reset();
    }
    m_selectedScheme = index;
    emit valueChanged(QPrivateSignal());
}

void FontSettingsAspect::copyColorScheme(const QString &name, QWidget *dialogParent)
{
    if (m_selectedScheme == -1)
        return;

    const ColorSchemeEntry &entry = m_schemeListModel.colorSchemeAt(m_selectedScheme);

    QString baseFileName = entry.filePath.completeBaseName();
    baseFileName += QLatin1String("_copy%1.xml");
    FilePath filePath = createColorSchemeFileName(baseFileName);

    if (!filePath.isEmpty()) {
        // Ask about saving any existing modifications
        maybeSaveColorScheme(dialogParent);

        // Make sure we're copying the current version
        m_value.setColorScheme(shownColorScheme());
        m_editedScheme.reset();

        ColorScheme scheme = m_value.colorScheme();
        scheme.setDisplayName(name);
        if (scheme.save(filePath)) {
            m_value.setColorScheme(scheme);
            m_value.setColorSchemeFileName(filePath);
        }

        refreshColorSchemeList();
        markSettingsDirty();
    }
}

void FontSettingsAspect::deleteColorScheme()
{
    const int index = m_selectedScheme;
    QTC_ASSERT(index != -1, return);

    const ColorSchemeEntry &entry = m_schemeListModel.colorSchemeAt(index);
    QTC_ASSERT(!entry.readOnly, return);

    if (entry.filePath.removeFile())
        m_schemeListModel.removeColorScheme(index);
}

void FontSettingsAspect::importScheme(const FilePath &importedFile, const QString &name)
{
    const Utils::FilePath saveFileName = createColorSchemeFileName(
        importedFile.baseName() + "%1." + importedFile.suffix());

    ColorScheme scheme;
    if (scheme.load(importedFile)) {
        scheme.setDisplayName(name);
        scheme.save(saveFileName);
        m_value.loadColorScheme(saveFileName, m_descriptions);
        m_editedScheme.reset();
    } else {
        qWarning() << "Failed to import color scheme:" << importedFile;
    }

    refreshColorSchemeList();
}

void FontSettingsAspect::exportScheme()
{
    if (m_selectedScheme == -1)
        return;

    const ColorSchemeEntry &entry = m_schemeListModel.colorSchemeAt(m_selectedScheme);

    const FilePath filePath
        = Utils::FileUtils::getSaveFilePath(Tr::tr("Export Color Scheme"),
                                            entry.filePath,
                                            Tr::tr("Color scheme (*.xml);;All files (*)"));

    if (!filePath.isEmpty())
        m_value.colorScheme().save(filePath);
}

void FontSettingsAspect::maybeSaveColorScheme(QWidget *dialogParent)
{
    if (!m_editedScheme || *m_editedScheme == m_value.colorScheme())
        return;

    QMessageBox
        messageBox(QMessageBox::Warning,
                   Tr::tr("Color Scheme Changed"),
                   Tr::tr("The color scheme \"%1\" was modified, do you want to save the changes?")
                       .arg(m_editedScheme->displayName()),
                   QMessageBox::Discard | QMessageBox::Save,
                   dialogParent);

    // Change the text of the discard button
    auto discardButton = static_cast<QPushButton*>(messageBox.button(QMessageBox::Discard));
    discardButton->setText(Tr::tr("Discard"));
    messageBox.addButton(discardButton, QMessageBox::DestructiveRole);
    messageBox.setDefaultButton(QMessageBox::Save);

    if (messageBox.exec() == QMessageBox::Save)
        m_editedScheme->save(m_value.colorSchemeFileName());
}

void FontSettingsAspect::refreshColorSchemeList()
{
    QList<ColorSchemeEntry> colorSchemes;

    const FilePath styleDir = Core::ICore::resourcePath("styles");

    FilePaths schemeList = styleDir.dirEntries(FileFilter({"*.xml"}, DirFilterFlag::Files));
    const FilePath defaultScheme = FontSettingsData::defaultSchemeFileName();

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

    const FilePaths files = customStylesPath().dirEntries(FileFilter({"*.xml"}, DirFilterFlag::Files));
    for (const FilePath &file : files) {
        if (m_value.colorSchemeFileName().fileName() == file.fileName())
            selected = colorSchemes.size();
        colorSchemes.append(ColorSchemeEntry(file, false));
    }

    m_refreshingSchemeList = true;
    m_schemeListModel.setColorSchemes(colorSchemes);
    selectColorScheme(colorSchemes.isEmpty() ? -1 : selected, nullptr);
    m_refreshingSchemeList = false;
}

bool FontSettingsAspect::isDirty() const
{
    return m_value != m_lastValue
        || (m_editedScheme && *m_editedScheme != m_value.colorScheme());
}

void FontSettingsAspect::apply()
{
    if (m_editedScheme && *m_editedScheme != m_value.colorScheme()) {
        // Update the scheme and save it under the name it already has
        m_value.setColorScheme(*m_editedScheme);
        m_value.colorScheme().save(m_value.colorSchemeFileName());
    }
    m_editedScheme.reset();

    if (m_selectedScheme != -1) {
        const ColorSchemeEntry &entry = m_schemeListModel.colorSchemeAt(m_selectedScheme);
        if (entry.filePath != m_value.colorSchemeFileName())
            m_value.loadColorScheme(entry.filePath, m_descriptions);
    }

    saveSettings();
    emit valueChanged(QPrivateSignal());
}

void FontSettingsAspect::saveSettings()
{
    m_lastValue = m_value;
    globalFontSettings().setData(m_value);
    globalFontSettings().apply();
}

void FontSettingsAspect::cancel()
{
    m_value = m_lastValue;
    m_editedScheme.reset();
    refreshColorSchemeList();
}

FontSettingsWidget::FontSettingsWidget(FontSettingsAspect *aspect)
    : m_aspect(aspect)
{
    m_antialias = new QCheckBox(Tr::tr("Antialias"));

    m_zoomSpinBox = new QSpinBox;
    m_zoomSpinBox->setSuffix(Tr::tr("%"));
    m_zoomSpinBox->setRange(10, 3000);
    m_zoomSpinBox->setSingleStep(10);

    m_lineSpacingSpinBox = new QSpinBox;
    m_lineSpacingSpinBox->setSuffix(Tr::tr("%"));
    m_lineSpacingSpinBox->setRange(50, 3000);

    m_lineSpacingWarningLabel = new QLabel;
    m_lineSpacingWarningLabel->setPixmap(Utils::Icons::WARNING.pixmap());
    m_lineSpacingWarningLabel->setToolTip(
                Tr::tr("A line spacing value other than 100% disables text wrapping.\n"
                       "A value less than 100% can result in overlapping and misaligned graphics."));

    m_fontComboBox = new QFontComboBox;

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
    m_schemeComboBox->setModel(m_aspect->schemeListModel());
    m_schemeComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    m_schemeEdit = new ColorSchemeEdit;
    m_schemeEdit->setFormatDescriptions(m_aspect->descriptions());

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
       },
       noMargin
    }.attachTo(this);

    connect(m_fontComboBox, &QFontComboBox::currentFontChanged, this, [this](const QFont &font) {
        m_aspect->changeValue([&font](FontSettingsData &value) { value.setFamily(font.family()); });
    });
    connect(m_sizeComboBox, &QComboBox::currentTextChanged, this, [this](const QString &text) {
        bool ok = false;
        const int size = text.toInt(&ok);
        if (ok)
            m_aspect->changeValue([size](FontSettingsData &value) { value.setFontSize(size); });
    });
    connect(m_zoomSpinBox, &QSpinBox::valueChanged, this, [this](int zoom) {
        m_aspect->changeValue([zoom](FontSettingsData &value) { value.setFontZoom(zoom); });
    });
    connect(m_antialias, &QCheckBox::toggled, this, [this](bool on) {
        m_aspect->changeValue([on](FontSettingsData &value) { value.setAntialias(on); });
    });
    connect(m_lineSpacingSpinBox, &QSpinBox::valueChanged, this, [this](int spacing) {
        m_aspect->changeValue([spacing](FontSettingsData &value) {
            value.setRelativeLineSpacing(spacing);
        });
    });
    connect(m_schemeComboBox, &QComboBox::currentIndexChanged, this, [this](int index) {
        m_aspect->selectColorScheme(index, window());
    });
    connect(m_schemeEdit, &ColorSchemeEdit::dirty, this, [this] {
        m_aspect->setEditedScheme(m_schemeEdit->colorScheme());
    });
    connect(m_copyButton, &QPushButton::clicked,
            this, &FontSettingsWidget::openCopyColorSchemeDialog);
    connect(m_schemeEdit, &ColorSchemeEdit::copyScheme,
            this, &FontSettingsWidget::openCopyColorSchemeDialog);
    connect(m_deleteButton, &QPushButton::clicked,
            this, &FontSettingsWidget::confirmDeleteColorScheme);
    connect(importButton, &QPushButton::clicked, this, &FontSettingsWidget::importScheme);
    connect(exportButton, &QPushButton::clicked, m_aspect, &FontSettingsAspect::exportScheme);
    connect(&globalFontSettings(), &FontSettings::changed, this, [this] {
        const int zoom = globalFontSettings().data().fontZoom();
        m_aspect->changeValue([zoom](FontSettingsData &value) { value.setFontZoom(zoom); });
    });
    connect(m_aspect, &FontSettingsAspect::valueChanged, this, &FontSettingsWidget::showValue);

    m_aspect->refreshColorSchemeList();
    showValue();

    installMarkSettingsDirtyTrigger(m_antialias);
    installMarkSettingsDirtyTrigger(m_zoomSpinBox);
    installMarkSettingsDirtyTrigger(m_lineSpacingSpinBox);
    installMarkSettingsDirtyTrigger(m_fontComboBox);
    installMarkSettingsDirtyTrigger(m_sizeComboBox);
    installMarkSettingsDirtyTrigger(m_schemeComboBox);
    connect(m_schemeEdit, &ColorSchemeEdit::dirty, [] { markSettingsDirty(); });
}

void FontSettingsWidget::showValue()
{
    const FontSettingsData &value = m_aspect->value();
    {
        const QSignalBlocker blocker(m_antialias);
        m_antialias->setChecked(value.antialias());
    }
    {
        const QSignalBlocker blocker(m_zoomSpinBox);
        m_zoomSpinBox->setValue(value.fontZoom());
    }
    {
        const QSignalBlocker blocker(m_lineSpacingSpinBox);
        m_lineSpacingSpinBox->setValue(value.relativeLineSpacing());
    }
    m_lineSpacingWarningLabel->setVisible(value.relativeLineSpacing() != 100);

    if (m_fontComboBox->currentFont().family() != value.family()) {
        const QSignalBlocker blocker(m_fontComboBox);
        m_fontComboBox->setCurrentFont(value.family());
    }
    bool ok = false;
    const int shownSize = m_sizeComboBox->currentText().toInt(&ok);
    if (m_pointSizesFamily != value.family() || !ok || shownSize != value.fontSize())
        updatePointSizes();

    {
        const QSignalBlocker blocker(m_schemeComboBox);
        m_schemeComboBox->setCurrentIndex(m_aspect->selectedScheme());
    }
    const bool readOnly = m_aspect->isSelectedSchemeReadOnly();
    m_copyButton->setEnabled(m_aspect->selectedScheme() != -1);
    m_deleteButton->setEnabled(!readOnly);
    m_schemeEdit->setReadOnly(readOnly);
    m_schemeEdit->setBaseFont(value.font());
    const ColorScheme shown = m_aspect->shownColorScheme();
    if (m_schemeEdit->colorScheme() != shown)
        m_schemeEdit->setColorScheme(shown);
}

void FontSettingsWidget::updatePointSizes()
{
    const QSignalBlocker blocker(m_sizeComboBox);
    m_pointSizesFamily = m_aspect->value().family();
    // Update point sizes
    const int oldSize = m_aspect->value().fontSize();
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

QList<int> FontSettingsWidget::pointSizesForSelectedFont() const
{
    const QString familyName = m_fontComboBox->currentFont().family();
    QList<int> sizeLst = QFontDatabase::pointSizes(familyName);
    if (!sizeLst.isEmpty())
        return sizeLst;

    QStringList styles = QFontDatabase::styles(familyName);
    if (!styles.isEmpty())
        sizeLst = QFontDatabase::pointSizes(familyName, styles.first());
    if (sizeLst.isEmpty())
        sizeLst = QFontDatabase::standardSizes();

    return sizeLst;
}

void FontSettingsWidget::openCopyColorSchemeDialog()
{
    QInputDialog *dialog = new QInputDialog(window());
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setInputMode(QInputDialog::TextInput);
    dialog->setWindowTitle(Tr::tr("Copy Color Scheme"));
    dialog->setLabelText(Tr::tr("Color scheme name:"));
    dialog->setTextValue(Tr::tr("%1 (copy)").arg(m_aspect->value().colorScheme().displayName()));

    connect(dialog, &QInputDialog::textValueSelected, this, [this](const QString &name) {
        m_aspect->copyColorScheme(name, window());
    });
    dialog->open();
}

void FontSettingsWidget::confirmDeleteColorScheme()
{
    if (m_aspect->selectedScheme() == -1 || m_aspect->isSelectedSchemeReadOnly())
        return;

    QMessageBox *messageBox = new QMessageBox(QMessageBox::Warning,
                                              Tr::tr("Delete Color Scheme"),
                                              Tr::tr("Are you sure you want to delete this color scheme permanently?"),
                                              QMessageBox::Discard | QMessageBox::Cancel,
                                              window());

    // Change the text and role of the discard button
    auto deleteButton = static_cast<QPushButton*>(messageBox->button(QMessageBox::Discard));
    deleteButton->setText(Tr::tr("Delete"));
    messageBox->addButton(deleteButton, QMessageBox::AcceptRole);
    messageBox->setDefaultButton(deleteButton);

    connect(messageBox, &QDialog::accepted, m_aspect, &FontSettingsAspect::deleteColorScheme);
    messageBox->setAttribute(Qt::WA_DeleteOnClose);
    messageBox->open();
}

void FontSettingsWidget::importScheme()
{
    const FilePath importedFile
        = Utils::FileUtils::getOpenFilePath(Tr::tr("Import Color Scheme"),
                                            {},
                                            Tr::tr("Color scheme (*.xml);;All files (*)"));

    if (importedFile.isEmpty())
        return;

    // Ask about saving any existing modifications
    m_aspect->maybeSaveColorScheme(window());

    QInputDialog *dialog = new QInputDialog(window());
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setInputMode(QInputDialog::TextInput);
    dialog->setWindowTitle(Tr::tr("Import Color Scheme"));
    dialog->setLabelText(Tr::tr("Color scheme name:"));
    dialog->setTextValue(importedFile.baseName());

    connect(dialog, &QInputDialog::textValueSelected, this, [this, importedFile](const QString &name) {
        m_aspect->importScheme(importedFile, name);
    });

    dialog->open();
}

class FontSettingsPageContainer final : public AspectContainer
{
public:
    explicit FontSettingsPageContainer(const FormatDescriptions &fd)
        : m_aspect(fd, this)
    {
        setLayouter([this] {
            using namespace Layouting;
            return Column { &m_aspect, noMargin };
        });
    }

private:
    FontSettingsAspect m_aspect;
};

class FontSettingsPage final : public Core::IOptionsPage
{
public:
    FontSettingsPage()
    {
        const FormatDescriptions fd = initialFormats();
        setupFontSettings(fd);
        setId(Constants::TEXT_EDITOR_FONT_SETTINGS);
        setDisplayName(Tr::tr("Font && Colors"));
        setCategory(TextEditor::Constants::TEXT_EDITOR_SETTINGS_CATEGORY);
        setSettingsProvider([fd] {
            static FontSettingsPageContainer thePage(fd);
            return &thePage;
        });
    }
};

void setupFontSettingsPage()
{
    static FontSettingsPage theFontSettingsPage;
}

#ifdef WITH_TESTS
class FontSettingsAspectTest final : public QObject
{
    Q_OBJECT

private slots:
    void testUpdatesEveryWidget()
    {
        FontSettingsAspect fontSettings(initialFormats());
        BaseAspect &aspect = fontSettings;
        const std::unique_ptr<QWidget> first(Layouting::Column{aspect}.emerge());
        const std::unique_ptr<QWidget> second(Layouting::Column{aspect}.emerge());
        const auto zoomSpinBox = [](const std::unique_ptr<QWidget> &widget) {
            return Utils::findOrDefault(widget->findChildren<QSpinBox *>(), [](QSpinBox *box) {
                return box->singleStep() == 10; // The line spacing steps by one.
            });
        };
        QSpinBox *firstZoom = zoomSpinBox(first);
        QSpinBox *secondZoom = zoomSpinBox(second);
        QVERIFY(firstZoom);
        QVERIFY(secondZoom);
        QVERIFY(!aspect.isDirty());
        const int before = firstZoom->value();
        const int changed = before == 100 ? 120 : 100;

        firstZoom->setValue(changed);
        QCOMPARE(secondZoom->value(), changed);
        QVERIFY(aspect.isDirty());

        aspect.cancel();
        QVERIFY(!aspect.isDirty());
        QCOMPARE(firstZoom->value(), before);
        QCOMPARE(secondZoom->value(), before);
    }

    void testCopyShowsTheNewName()
    {
        // Without a widget, as the combo boxes of a widget reload the scheme on their own.
        FontSettingsAspect fontSettings(initialFormats());
        fontSettings.refreshColorSchemeList();
        const FilePath original = fontSettings.value().colorSchemeFileName();

        fontSettings.copyColorScheme("tst_FontSettingsAspect copy", nullptr);
        const FilePath copy = fontSettings.value().colorSchemeFileName();
        const auto removeCopy = qScopeGuard([copy, original] {
            if (copy != original)
                copy.removeFile();
        });

        QVERIFY(copy != original);
        QCOMPARE(fontSettings.value().colorScheme().displayName(),
                 "tst_FontSettingsAspect copy");
        QCOMPARE(fontSettings.shownColorScheme().displayName(), "tst_FontSettingsAspect copy");
    }

    void testKeepsSchemeEditsForANewWidget()
    {
        FontSettingsAspect fontSettings(initialFormats());
        BaseAspect &aspect = fontSettings;
        const std::unique_ptr<QWidget> first(Layouting::Column{aspect}.emerge());
        ColorScheme edited = fontSettings.shownColorScheme();
        Format format = edited.formatFor(C_TEXT);
        format.setForeground(format.foreground() == QColor(1, 2, 3) ? QColor(3, 2, 1)
                                                                   : QColor(1, 2, 3));
        edited.setFormatFor(C_TEXT, format);
        fontSettings.setEditedScheme(edited);
        QVERIFY(aspect.isDirty());

        const std::unique_ptr<QWidget> second(Layouting::Column{aspect}.emerge());

        QVERIFY(aspect.isDirty());
        for (const std::unique_ptr<QWidget> *widget : {&first, &second}) {
            const auto edit = (*widget)->findChild<ColorSchemeEdit *>();
            QVERIFY(edit);
            QVERIFY(edit->colorScheme() == edited);
        }
    }
};

QObject *createFontSettingsAspectTest()
{
    return new FontSettingsAspectTest;
}
#endif // WITH_TESTS

} // namespace Internal
} // namespace TextEditor

#include "fontsettingspage.moc"
