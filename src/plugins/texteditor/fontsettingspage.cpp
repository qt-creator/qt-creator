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

#include "fontsettingspage.h"

#include "colorschemeedit.h"
#include "fontsettings.h"
#include "texteditorconstants.h"
#include "ui_fontsettingspage.h"

#include <coreplugin/icore.h>
#include <utils/stringutils.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QSettings>
#include <QTimer>
#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFontDatabase>
#include <QInputDialog>
#include <QListWidget>
#include <QMessageBox>
#include <QPalette>
#include <QTextCharFormat>
#include <QTextEdit>
#include <QToolButton>

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
    SchemeListModel(QObject *parent = 0):
        QAbstractListModel(parent)
    {
    }

    int rowCount(const QModelIndex &parent) const
    { return parent.isValid() ? 0 : m_colorSchemes.size(); }

    QVariant data(const QModelIndex &index, int role) const
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


class FontSettingsPagePrivate
{
public:
    FontSettingsPagePrivate(const TextEditor::FormatDescriptions &fd,
                            Core::Id id,
                            const QString &displayName,
                            const QString &category);
    ~FontSettingsPagePrivate();

public:
    const Core::Id m_id;
    const QString m_displayName;
    const QString m_settingsGroup;

    TextEditor::FormatDescriptions m_descriptions;
    FontSettings m_value;
    FontSettings m_lastValue;
    Ui::FontSettingsPage *m_ui;
    SchemeListModel *m_schemeListModel;
    bool m_refreshingSchemeList;
    QString m_searchKeywords;
};

} // namespace Internal
} // namespace TextEditor

using namespace TextEditor;
using namespace TextEditor::Internal;

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

// ------- FontSettingsPagePrivate
FontSettingsPagePrivate::FontSettingsPagePrivate(const TextEditor::FormatDescriptions &fd,
                                                 Core::Id id,
                                                 const QString &displayName,
                                                 const QString &category) :
    m_id(id),
    m_displayName(displayName),
    m_settingsGroup(Utils::settingsKey(category)),
    m_descriptions(fd),
    m_ui(0),
    m_schemeListModel(new SchemeListModel),
    m_refreshingSchemeList(false)
{
    bool settingsFound = false;
    QSettings *settings = Core::ICore::settings();
    if (settings)
        settingsFound = m_value.fromSettings(m_settingsGroup, m_descriptions, settings);

    if (!settingsFound) { // Apply defaults
        foreach (const FormatDescription &f, m_descriptions) {
            Format &format = m_value.formatFor(f.id());
            format.setForeground(f.foreground());
            format.setBackground(f.background());
            format.setBold(f.format().bold());
            format.setItalic(f.format().italic());
        }
    } else if (m_value.colorSchemeFileName().isEmpty()) {
        // No color scheme was loaded, but one might be imported from the ini file
        ColorScheme defaultScheme;
        foreach (const FormatDescription &f, m_descriptions) {
            Format &format = defaultScheme.formatFor(f.id());
            format.setForeground(f.foreground());
            format.setBackground(f.background());
            format.setBold(f.format().bold());
            format.setItalic(f.format().italic());
        }
        if (m_value.colorScheme() != defaultScheme) {
            // Save it as a color scheme file
            QString schemeFileName = createColorSchemeFileName(QLatin1String("customized%1.xml"));
            if (!schemeFileName.isEmpty()) {
                if (m_value.saveColorScheme(schemeFileName) && settings)
                    m_value.toSettings(m_settingsGroup, settings);
            }
        }
    }

    m_lastValue = m_value;
}

FontSettingsPagePrivate::~FontSettingsPagePrivate()
{
    delete m_schemeListModel;
}


// ------- FormatDescription
FormatDescription::FormatDescription(TextStyle id, const QString &displayName, const QString &tooltipText, const QColor &foreground) :
    m_id(id),
    m_displayName(displayName),
    m_tooltipText(tooltipText)
{
    m_format.setForeground(foreground);
}

FormatDescription::FormatDescription(TextStyle id, const QString &displayName, const QString &tooltipText, const Format &format) :
    m_id(id),
    m_displayName(displayName),
    m_format(format),
    m_tooltipText(tooltipText)
{
}

QColor FormatDescription::foreground() const
{
    if (m_id == C_LINE_NUMBER) {
        const QColor bg = QApplication::palette().background().color();
        if (bg.value() < 128)
            return QApplication::palette().foreground().color();
        else
            return QApplication::palette().dark().color();
    } else if (m_id == C_CURRENT_LINE_NUMBER) {
        const QColor bg = QApplication::palette().background().color();
        if (bg.value() < 128)
            return QApplication::palette().foreground().color();
        else
            return m_format.foreground();
    } else if (m_id == C_OCCURRENCES_UNUSED) {
        return Qt::darkYellow;
    } else if (m_id == C_PARENTHESES) {
        return QColor(Qt::red);
    }
    return m_format.foreground();
}

QColor FormatDescription::background() const
{
    if (m_id == C_TEXT)
        return Qt::white;
    else if (m_id == C_LINE_NUMBER)
        return QApplication::palette().background().color();
    else if (m_id == C_SEARCH_RESULT)
        return QColor(0xffef0b);
    else if (m_id == C_PARENTHESES)
        return QColor(0xb4, 0xee, 0xb4);
    else if (m_id == C_CURRENT_LINE || m_id == C_SEARCH_SCOPE) {
        const QPalette palette = QApplication::palette();
        const QColor &fg = palette.color(QPalette::Highlight);
        const QColor &bg = palette.color(QPalette::Base);

        qreal smallRatio;
        qreal largeRatio;
        if (m_id == C_CURRENT_LINE) {
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
    } else if (m_id == C_SELECTION) {
        const QPalette palette = QApplication::palette();
        return palette.color(QPalette::Highlight);
    } else if (m_id == C_OCCURRENCES) {
        return QColor(180, 180, 180);
    } else if (m_id == C_OCCURRENCES_RENAME) {
        return QColor(255, 100, 100);
    } else if (m_id == C_DISABLED_CODE) {
        return QColor(239, 239, 239);
    }
    return QColor(); // invalid color
}


//  ------------ FontSettingsPage
FontSettingsPage::FontSettingsPage(const FormatDescriptions &fd,
                                   Core::Id id,
                                   QObject *parent) :
    TextEditorOptionsPage(parent),
    d_ptr(new FontSettingsPagePrivate(fd, id, tr("Font && Colors"), category().toString()))
{
    setId(d_ptr->m_id);
    setDisplayName(d_ptr->m_displayName);
}

FontSettingsPage::~FontSettingsPage()
{
    delete d_ptr;
}

QWidget *FontSettingsPage::createPage(QWidget *parent)
{
    QWidget *w = new QWidget(parent);
    d_ptr->m_ui = new Ui::FontSettingsPage;
    d_ptr->m_ui->setupUi(w);
    d_ptr->m_ui->schemeComboBox->setModel(d_ptr->m_schemeListModel);

    QFontDatabase db;
    const QStringList families = db.families();
    d_ptr->m_ui->familyComboBox->addItems(families);
    const int idx = families.indexOf(d_ptr->m_value.family());
    d_ptr->m_ui->familyComboBox->setCurrentIndex(idx);

    d_ptr->m_ui->antialias->setChecked(d_ptr->m_value.antialias());
    d_ptr->m_ui->zoomSpinBox->setValue(d_ptr->m_value.fontZoom());

    d_ptr->m_ui->schemeEdit->setFormatDescriptions(d_ptr->m_descriptions);
    d_ptr->m_ui->schemeEdit->setBaseFont(d_ptr->m_value.font());
    d_ptr->m_ui->schemeEdit->setColorScheme(d_ptr->m_value.colorScheme());

    connect(d_ptr->m_ui->familyComboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(fontFamilySelected(QString)));
    connect(d_ptr->m_ui->sizeComboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(fontSizeSelected(QString)));
    connect(d_ptr->m_ui->zoomSpinBox, SIGNAL(valueChanged(int)), this, SLOT(fontZoomChanged()));
    connect(d_ptr->m_ui->schemeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(colorSchemeSelected(int)));
    connect(d_ptr->m_ui->copyButton, SIGNAL(clicked()), this, SLOT(copyColorScheme()));
    connect(d_ptr->m_ui->deleteButton, SIGNAL(clicked()), this, SLOT(confirmDeleteColorScheme()));


    updatePointSizes();
    refreshColorSchemeList();
    d_ptr->m_lastValue = d_ptr->m_value;
    if (d_ptr->m_searchKeywords.isEmpty()) {
        QLatin1Char sep(' ');
        d_ptr->m_searchKeywords =
                d_ptr->m_ui->fontGroupBox->title() + sep
                + d_ptr->m_ui->familyLabel->text() + sep
                + d_ptr->m_ui->sizeLabel->text() + sep
                + d_ptr->m_ui->zoomLabel->text() + sep
                + d_ptr->m_ui->antialias->text() + sep
                + d_ptr->m_ui->colorSchemeGroupBox->title();
        d_ptr->m_searchKeywords.remove(QLatin1Char('&'));
    }
    return w;
}

void FontSettingsPage::fontFamilySelected(const QString &family)
{
    d_ptr->m_value.setFamily(family);
    d_ptr->m_ui->schemeEdit->setBaseFont(d_ptr->m_value.font());
    updatePointSizes();
}

void FontSettingsPage::updatePointSizes()
{
    // Update point sizes
    const int oldSize = d_ptr->m_value.fontSize();
    d_ptr->m_ui->sizeComboBox->clear();
    const QList<int> sizeLst = pointSizesForSelectedFont();
    int idx = -1;
    int i = 0;
    for (; i < sizeLst.count(); ++i) {
        if (idx == -1 && sizeLst.at(i) >= oldSize)
            idx = i;
        d_ptr->m_ui->sizeComboBox->addItem(QString::number(sizeLst.at(i)));
    }
    if (idx != -1)
        d_ptr->m_ui->sizeComboBox->setCurrentIndex(idx);
}

QList<int> FontSettingsPage::pointSizesForSelectedFont() const
{
    QFontDatabase db;
    const QString familyName = d_ptr->m_ui->familyComboBox->currentText();
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

void FontSettingsPage::fontSizeSelected(const QString &sizeString)
{
    bool ok = true;
    const int size = sizeString.toInt(&ok);
    if (ok) {
        d_ptr->m_value.setFontSize(size);
        d_ptr->m_ui->schemeEdit->setBaseFont(d_ptr->m_value.font());
    }
}

void FontSettingsPage::fontZoomChanged()
{
    d_ptr->m_value.setFontZoom(d_ptr->m_ui->zoomSpinBox->value());
}

void FontSettingsPage::colorSchemeSelected(int index)
{
    bool readOnly = true;
    if (index != -1) {
        // Check whether we're switching away from a changed color scheme
        if (!d_ptr->m_refreshingSchemeList)
            maybeSaveColorScheme();

        const ColorSchemeEntry &entry = d_ptr->m_schemeListModel->colorSchemeAt(index);
        readOnly = entry.readOnly;
        d_ptr->m_value.loadColorScheme(entry.fileName, d_ptr->m_descriptions);
        d_ptr->m_ui->schemeEdit->setColorScheme(d_ptr->m_value.colorScheme());
    }
    d_ptr->m_ui->copyButton->setEnabled(index != -1);
    d_ptr->m_ui->deleteButton->setEnabled(!readOnly);
    d_ptr->m_ui->schemeEdit->setReadOnly(readOnly);
}

void FontSettingsPage::copyColorScheme()
{
    QInputDialog *dialog = new QInputDialog(d_ptr->m_ui->copyButton->window());
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setInputMode(QInputDialog::TextInput);
    dialog->setWindowTitle(tr("Copy Color Scheme"));
    dialog->setLabelText(tr("Color scheme name:"));
    dialog->setTextValue(tr("%1 (copy)").arg(d_ptr->m_value.colorScheme().displayName()));

    connect(dialog, SIGNAL(textValueSelected(QString)), this, SLOT(copyColorScheme(QString)));
    dialog->open();
}

void FontSettingsPage::copyColorScheme(const QString &name)
{
    int index = d_ptr->m_ui->schemeComboBox->currentIndex();
    if (index == -1)
        return;

    const ColorSchemeEntry &entry = d_ptr->m_schemeListModel->colorSchemeAt(index);

    QString baseFileName = QFileInfo(entry.fileName).completeBaseName();
    baseFileName += QLatin1String("_copy%1.xml");
    QString fileName = createColorSchemeFileName(baseFileName);

    if (!fileName.isEmpty()) {
        // Ask about saving any existing modifactions
        maybeSaveColorScheme();

        // Make sure we're copying the current version
        d_ptr->m_value.setColorScheme(d_ptr->m_ui->schemeEdit->colorScheme());

        ColorScheme scheme = d_ptr->m_value.colorScheme();
        scheme.setDisplayName(name);
        if (scheme.save(fileName, Core::ICore::mainWindow()))
            d_ptr->m_value.setColorSchemeFileName(fileName);

        refreshColorSchemeList();
    }
}

void FontSettingsPage::confirmDeleteColorScheme()
{
    const int index = d_ptr->m_ui->schemeComboBox->currentIndex();
    if (index == -1)
        return;

    const ColorSchemeEntry &entry = d_ptr->m_schemeListModel->colorSchemeAt(index);
    if (entry.readOnly)
        return;

    QMessageBox *messageBox = new QMessageBox(QMessageBox::Warning,
                                              tr("Delete Color Scheme"),
                                              tr("Are you sure you want to delete this color scheme permanently?"),
                                              QMessageBox::Discard | QMessageBox::Cancel,
                                              d_ptr->m_ui->deleteButton->window());

    // Change the text and role of the discard button
    QPushButton *deleteButton = static_cast<QPushButton*>(messageBox->button(QMessageBox::Discard));
    deleteButton->setText(tr("Delete"));
    messageBox->addButton(deleteButton, QMessageBox::AcceptRole);
    messageBox->setDefaultButton(deleteButton);

    connect(deleteButton, SIGNAL(clicked()), messageBox, SLOT(accept()));
    connect(messageBox, SIGNAL(accepted()), this, SLOT(deleteColorScheme()));
    messageBox->setAttribute(Qt::WA_DeleteOnClose);
    messageBox->open();
}

void FontSettingsPage::deleteColorScheme()
{
    const int index = d_ptr->m_ui->schemeComboBox->currentIndex();
    QTC_ASSERT(index != -1, return);

    const ColorSchemeEntry &entry = d_ptr->m_schemeListModel->colorSchemeAt(index);
    QTC_ASSERT(!entry.readOnly, return);

    if (QFile::remove(entry.fileName))
        d_ptr->m_schemeListModel->removeColorScheme(index);
}

void FontSettingsPage::maybeSaveColorScheme()
{
    if (d_ptr->m_value.colorScheme() == d_ptr->m_ui->schemeEdit->colorScheme())
        return;

    QMessageBox *messageBox = new QMessageBox(QMessageBox::Warning,
                                              tr("Color Scheme Changed"),
                                              tr("The color scheme \"%1\" was modified, do you want to save the changes?")
                                                  .arg(d_ptr->m_ui->schemeEdit->colorScheme().displayName()),
                                              QMessageBox::Discard | QMessageBox::Save,
                                              d_ptr->m_ui->schemeComboBox->window());

    // Change the text of the discard button
    QPushButton *discardButton = static_cast<QPushButton*>(messageBox->button(QMessageBox::Discard));
    discardButton->setText(tr("Discard"));
    messageBox->addButton(discardButton, QMessageBox::DestructiveRole);
    messageBox->setDefaultButton(QMessageBox::Save);

    if (messageBox->exec() == QMessageBox::Save) {
        const ColorScheme &scheme = d_ptr->m_ui->schemeEdit->colorScheme();
        scheme.save(d_ptr->m_value.colorSchemeFileName(), Core::ICore::mainWindow());
    }
}

void FontSettingsPage::refreshColorSchemeList()
{
    QList<ColorSchemeEntry> colorSchemes;

    QString resourcePath = Core::ICore::resourcePath();
    QDir styleDir(resourcePath + QLatin1String("/styles"));
    styleDir.setNameFilters(QStringList() << QLatin1String("*.xml"));
    styleDir.setFilter(QDir::Files);

    int selected = 0;

    QStringList schemeList = styleDir.entryList();
    QString defaultScheme = QFileInfo(FontSettings::defaultSchemeFileName()).fileName();
    if (schemeList.removeAll(defaultScheme))
        schemeList.prepend(defaultScheme);
    foreach (const QString &file, schemeList) {
        const QString fileName = styleDir.absoluteFilePath(file);
        if (d_ptr->m_value.colorSchemeFileName() == fileName)
            selected = colorSchemes.size();
        colorSchemes.append(ColorSchemeEntry(fileName, true));
    }

    if (colorSchemes.isEmpty())
        qWarning() << "Warning: no color schemes found in path:" << styleDir.path();

    styleDir.setPath(customStylesPath());

    foreach (const QString &file, styleDir.entryList()) {
        const QString fileName = styleDir.absoluteFilePath(file);
        if (d_ptr->m_value.colorSchemeFileName() == fileName)
            selected = colorSchemes.size();
        colorSchemes.append(ColorSchemeEntry(fileName, false));
    }

    d_ptr->m_refreshingSchemeList = true;
    d_ptr->m_schemeListModel->setColorSchemes(colorSchemes);
    d_ptr->m_ui->schemeComboBox->setCurrentIndex(selected);
    d_ptr->m_refreshingSchemeList = false;
}

void FontSettingsPage::delayedChange()
{
    emit changed(d_ptr->m_value);
}

void FontSettingsPage::apply()
{
    if (!d_ptr->m_ui) // page was never shown
        return;
    d_ptr->m_value.setAntialias(d_ptr->m_ui->antialias->isChecked());

    if (d_ptr->m_value.colorScheme() != d_ptr->m_ui->schemeEdit->colorScheme()) {
        // Update the scheme and save it under the name it already has
        d_ptr->m_value.setColorScheme(d_ptr->m_ui->schemeEdit->colorScheme());
        const ColorScheme &scheme = d_ptr->m_value.colorScheme();
        scheme.save(d_ptr->m_value.colorSchemeFileName(), Core::ICore::mainWindow());
    }

    int index = d_ptr->m_ui->schemeComboBox->currentIndex();
    if (index != -1) {
        const ColorSchemeEntry &entry = d_ptr->m_schemeListModel->colorSchemeAt(index);
        if (entry.fileName != d_ptr->m_value.colorSchemeFileName())
            d_ptr->m_value.loadColorScheme(entry.fileName, d_ptr->m_descriptions);
    }

    saveSettings();
}

void FontSettingsPage::saveSettings()
{
    if (d_ptr->m_value != d_ptr->m_lastValue) {
        d_ptr->m_lastValue = d_ptr->m_value;
        d_ptr->m_value.toSettings(d_ptr->m_settingsGroup, Core::ICore::settings());

        QTimer::singleShot(0, this, SLOT(delayedChange()));
    }
}

void FontSettingsPage::finish()
{
    if (!d_ptr->m_ui) // page was never shown
        return;
    // If changes were applied, these are equal. Otherwise restores last value.
    d_ptr->m_value = d_ptr->m_lastValue;
    delete d_ptr->m_ui;
    d_ptr->m_ui = 0;
}

const FontSettings &FontSettingsPage::fontSettings() const
{
    return d_ptr->m_value;
}

bool FontSettingsPage::matches(const QString &s) const
{
    return d_ptr->m_searchKeywords.contains(s, Qt::CaseInsensitive);
}
