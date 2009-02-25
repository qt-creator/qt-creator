/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "fontsettingspage.h"
#include "fontsettings.h"
#include "texteditorconstants.h"
#include "ui_fontsettingspage.h"

#include <coreplugin/icore.h>
#include <utils/settingsutils.h>

#include <QtCore/QSettings>
#include <QtCore/QTimer>
#include <QtGui/QCheckBox>
#include <QtGui/QColorDialog>
#include <QtGui/QComboBox>
#include <QtGui/QFontDatabase>
#include <QtGui/QListWidget>
#include <QtGui/QPalette>
#include <QtGui/QPalette>
#include <QtGui/QTextCharFormat>
#include <QtGui/QTextEdit>
#include <QtGui/QToolButton>

static inline QString colorButtonStyleSheet(const QColor &bgColor)
{
    if (bgColor.isValid()) {
        QString rc = QLatin1String("border: 2px solid black; border-radius: 2px; background:");
        rc += bgColor.name();
        return rc;
    }
    return QLatin1String("border: 2px dotted black; border-radius: 2px;");
}

namespace TextEditor {
namespace Internal {

class FontSettingsPagePrivate
{
public:
    FontSettingsPagePrivate(const TextEditor::FormatDescriptions &fd,
                            const QString &name,
                            const QString &category,
                            const QString &trCategory);

public:
    const QString m_name;
    const QString m_settingsGroup;
    const QString m_category;
    const QString m_trCategory;

    TextEditor::FormatDescriptions m_descriptions;
    FontSettings m_value;
    FontSettings m_lastValue;
    int m_curItem;
    Ui::FontSettingsPage ui;
};

FontSettingsPagePrivate::FontSettingsPagePrivate(const TextEditor::FormatDescriptions &fd,
                                                 const QString &name,
                                                 const QString &category,
                                                 const QString &trCategory) :
    m_name(name),
    m_settingsGroup(Core::Utils::settingsKey(category)),
    m_category(category),
    m_trCategory(trCategory),
    m_descriptions(fd),
    m_value(fd),
    m_lastValue(fd),
    m_curItem(-1)
{
    bool settingsFound = false;
    if (const QSettings *settings = Core::ICore::instance()->settings())
        settingsFound = m_value.fromSettings(m_settingsGroup, m_descriptions, settings);
    if (!settingsFound) { // Apply defaults
        foreach (const FormatDescription &f, m_descriptions) {
            const QString name = f.name();
            m_lastValue.formatFor(name).setForeground(f.foreground());
            m_lastValue.formatFor(name).setBackground(f.background());
            m_value.formatFor(name).setForeground(f.foreground());
            m_value.formatFor(name).setBackground(f.background());
        }
    }

    m_lastValue = m_value;
}

} // namespace Internal
} // namespace TextEditor

using namespace TextEditor;
using namespace TextEditor::Internal;

// ------- FormatDescription
FormatDescription::FormatDescription(const QString &name, const QString &trName, const QColor &color) :
    m_name(name),
    m_trName(trName)
{
    m_format.setForeground(color);
}

QString FormatDescription::name() const
{
    return m_name;
}

QString FormatDescription::trName() const
{
    return m_trName;
}

QColor FormatDescription::foreground() const
{
    if (m_name == QLatin1String("LineNumber"))
        return QApplication::palette().dark().color();
    if (m_name == QLatin1String("Parentheses"))
        return QColor(Qt::red);
    return m_format.foreground();
}

void FormatDescription::setForeground(const QColor &foreground)
{
    m_format.setForeground(foreground);
}

QColor FormatDescription::background() const
{
    if (m_name == QLatin1String(Constants::C_TEXT))
        return Qt::white;
    else if (m_name == QLatin1String(Constants::C_LINE_NUMBER))
        return QApplication::palette().background().color();
    else if (m_name == QLatin1String(Constants::C_SEARCH_RESULT))
        return QColor(0xffef0b);
    else if (m_name == QLatin1String(Constants::C_PARENTHESES))
        return QColor(0xb4, 0xee, 0xb4);
    else if (m_name == QLatin1String(Constants::C_CURRENT_LINE)
             || m_name == QLatin1String(Constants::C_SEARCH_SCOPE)) {
        const QPalette palette = QApplication::palette();
        const QColor &fg = palette.color(QPalette::Highlight);
        const QColor &bg = palette.color(QPalette::Base);

        qreal smallRatio;
        qreal largeRatio;
        if (m_name == QLatin1String(Constants::C_CURRENT_LINE)) {
            smallRatio = .15;
            largeRatio = .3;
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
    } else if (m_name == QLatin1String(Constants::C_SELECTION)) {
        const QPalette palette = QApplication::palette();
        return palette.color(QPalette::Highlight);
    }
    return QColor(); // invalid color
}


//  ------------ FontSettingsPage
FontSettingsPage::FontSettingsPage(const FormatDescriptions &fd,
                                   const QString &category,
                                   const QString &trCategory,
                                   QObject *parent) :
    Core::IOptionsPage(parent),
    d_ptr(new FontSettingsPagePrivate(fd, tr("Font & Colors"), category, trCategory))
{
}

FontSettingsPage::~FontSettingsPage()
{
    delete d_ptr;
}

QString FontSettingsPage::name() const
{
    return d_ptr->m_name;
}

QString FontSettingsPage::category() const
{
    return d_ptr->m_category;
}

QString FontSettingsPage::trCategory() const
{
    return d_ptr->m_trCategory;
}

QWidget *FontSettingsPage::createPage(QWidget *parent)
{
    QWidget *w = new QWidget(parent);
    d_ptr->ui.setupUi(w);

    d_ptr->ui.itemListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);

    foreach (const FormatDescription &d, d_ptr->m_descriptions)
        d_ptr->ui.itemListWidget->addItem(d.trName());

    QFontDatabase db;
    const QStringList families = db.families();
    d_ptr->ui.familyComboBox->addItems(families);
    const int idx = families.indexOf(d_ptr->m_value.family());
    d_ptr->ui.familyComboBox->setCurrentIndex(idx);

    connect(d_ptr->ui.familyComboBox, SIGNAL(activated(int)), this, SLOT(updatePointSizes()));
    connect(d_ptr->ui.sizeComboBox, SIGNAL(activated(int)), this, SLOT(updatePreview()));
    connect(d_ptr->ui.itemListWidget, SIGNAL(itemSelectionChanged()),
        this, SLOT(itemChanged()));
    connect(d_ptr->ui.foregroundToolButton, SIGNAL(clicked()),
        this, SLOT(changeForeColor()));
    connect(d_ptr->ui.backgroundToolButton, SIGNAL(clicked()),
        this, SLOT(changeBackColor()));
    connect(d_ptr->ui.eraseBackgroundToolButton, SIGNAL(clicked()),
        this, SLOT(eraseBackColor()));
    connect(d_ptr->ui.boldCheckBox, SIGNAL(toggled(bool)), this, SLOT(checkCheckBoxes()));
    connect(d_ptr->ui.italicCheckBox, SIGNAL(toggled(bool)), this, SLOT(checkCheckBoxes()));

    if (!d_ptr->m_descriptions.empty())
        d_ptr->ui.itemListWidget->setCurrentRow(0);

    updatePointSizes();
    d_ptr->m_lastValue = d_ptr->m_value;
    return w;
}

void FontSettingsPage::itemChanged()
{
    QListWidgetItem *item = d_ptr->ui.itemListWidget->currentItem();
    if (!item)
        return;

    const int numFormats = d_ptr->m_descriptions.size();
    for (int i = 0; i < numFormats; i++) {
        if (d_ptr->m_descriptions[i].trName() == item->text()) {
            d_ptr->m_curItem = i;
            const Format &format = d_ptr->m_value.formatFor(d_ptr->m_descriptions[i].name());
            d_ptr->ui.foregroundToolButton->setStyleSheet(colorButtonStyleSheet(format.foreground()));
            d_ptr->ui.backgroundToolButton->setStyleSheet(colorButtonStyleSheet(format.background()));

            d_ptr->ui.eraseBackgroundToolButton->setEnabled(i > 0 && format.background().isValid());

            const bool boldBlocked = d_ptr->ui.boldCheckBox->blockSignals(true);
            d_ptr->ui.boldCheckBox->setChecked(format.bold());
            d_ptr->ui.boldCheckBox->blockSignals(boldBlocked);
            const bool italicBlocked = d_ptr->ui.italicCheckBox->blockSignals(true);
            d_ptr->ui.italicCheckBox->setChecked(format.italic());
            d_ptr->ui.italicCheckBox->blockSignals(italicBlocked);
            updatePreview();
            break;
        }
    }
}

void FontSettingsPage::changeForeColor()
{
    if (d_ptr->m_curItem == -1)
        return;
    QColor color = d_ptr->m_value.formatFor(d_ptr->m_descriptions[d_ptr->m_curItem].name()).foreground();
    const QColor newColor = QColorDialog::getColor(color, d_ptr->ui.boldCheckBox->window());
    if (!newColor.isValid())
        return;
    QPalette p = d_ptr->ui.foregroundToolButton->palette();
    p.setColor(QPalette::Active, QPalette::Button, newColor);
    d_ptr->ui.foregroundToolButton->setStyleSheet(colorButtonStyleSheet(newColor));

    const int numFormats = d_ptr->m_descriptions.size();
    for (int i = 0; i < numFormats; i++) {
        QList<QListWidgetItem*> items = d_ptr->ui.itemListWidget->findItems(d_ptr->m_descriptions[i].trName(), Qt::MatchExactly);
        if (!items.isEmpty() && items.first()->isSelected())
            d_ptr->m_value.formatFor(d_ptr->m_descriptions[i].name()).setForeground(newColor);
    }

    updatePreview();
}

void FontSettingsPage::changeBackColor()
{
    if (d_ptr->m_curItem == -1)
        return;
    QColor color = d_ptr->m_value.formatFor(d_ptr->m_descriptions[d_ptr->m_curItem].name()).background();
    const QColor newColor = QColorDialog::getColor(color, d_ptr->ui.boldCheckBox->window());
    if (!newColor.isValid())
        return;
    d_ptr->ui.backgroundToolButton->setStyleSheet(colorButtonStyleSheet(newColor));

    const int numFormats = d_ptr->m_descriptions.size();
    for (int i = 0; i < numFormats; i++) {
        QList<QListWidgetItem*> items = d_ptr->ui.itemListWidget->findItems(d_ptr->m_descriptions[i].trName(), Qt::MatchExactly);
        if (!items.isEmpty() && items.first()->isSelected())
            d_ptr->m_value.formatFor(d_ptr->m_descriptions[i].name()).setBackground(newColor);
    }

    updatePreview();
}

void FontSettingsPage::eraseBackColor()
{
    if (d_ptr->m_curItem == -1)
        return;
    QColor newColor;
    d_ptr->ui.backgroundToolButton->setStyleSheet(colorButtonStyleSheet(newColor));

    const int numFormats = d_ptr->m_descriptions.size();
    for (int i = 0; i < numFormats; i++) {
        QList<QListWidgetItem*> items = d_ptr->ui.itemListWidget->findItems(d_ptr->m_descriptions[i].trName(), Qt::MatchExactly);
        if (!items.isEmpty() && items.first()->isSelected())
            d_ptr->m_value.formatFor(d_ptr->m_descriptions[i].name()).setBackground(newColor);
    }

    updatePreview();
}

void FontSettingsPage::checkCheckBoxes()
{
    if (d_ptr->m_curItem == -1)
        return;
    const int numFormats = d_ptr->m_descriptions.size();
    for (int i = 0; i < numFormats; i++) {
        QList<QListWidgetItem*> items = d_ptr->ui.itemListWidget->findItems(d_ptr->m_descriptions[i].trName(), Qt::MatchExactly);
        if (!items.isEmpty() && items.first()->isSelected()) {
            d_ptr->m_value.formatFor(d_ptr->m_descriptions[i].name()).setBold(d_ptr->ui.boldCheckBox->isChecked());
            d_ptr->m_value.formatFor(d_ptr->m_descriptions[i].name()).setItalic(d_ptr->ui.italicCheckBox->isChecked());
        }
    }
    updatePreview();
}

void FontSettingsPage::updatePreview()
{
    if (d_ptr->m_curItem == -1)
        return;

    const Format &currentFormat = d_ptr->m_value.formatFor(d_ptr->m_descriptions[d_ptr->m_curItem].name());
    const Format &baseFormat = d_ptr->m_value.formatFor(QLatin1String("Text"));

    QPalette pal = QApplication::palette();
    if (baseFormat.foreground().isValid()) {
        pal.setColor(QPalette::Text, baseFormat.foreground());
        pal.setColor(QPalette::Foreground, baseFormat.foreground());
    }
    if (baseFormat.background().isValid())
        pal.setColor(QPalette::Base, baseFormat.background());

    d_ptr->ui.previewTextEdit->setPalette(pal);

    QTextCharFormat format;
    if (currentFormat.foreground().isValid())
        format.setForeground(QBrush(currentFormat.foreground()));
    if (currentFormat.background().isValid())
        format.setBackground(QBrush(currentFormat.background()));
    format.setFontFamily(d_ptr->ui.familyComboBox->currentText());
    bool ok;
    int size = d_ptr->ui.sizeComboBox->currentText().toInt(&ok);
    if (!ok) {
        size = QFont().pointSize();
    }
    format.setFontPointSize(size);
    format.setFontItalic(currentFormat.italic());
    if (currentFormat.bold())
        format.setFontWeight(QFont::Bold);
    d_ptr->ui.previewTextEdit->setCurrentCharFormat(format);

    d_ptr->ui.previewTextEdit->setPlainText(tr("\n\tThis is only an example."));
}

void FontSettingsPage::updatePointSizes()
{
    const int oldSize = d_ptr->m_value.fontSize();
    if (d_ptr->ui.sizeComboBox->count()) {
        const QString curSize = d_ptr->ui.sizeComboBox->currentText();
        bool ok = true;
        int oldSize = curSize.toInt(&ok);
        if (!ok)
            oldSize = d_ptr->m_value.fontSize();
        d_ptr->ui.sizeComboBox->clear();
    }
    QFontDatabase db;
    const QList<int> sizeLst = db.pointSizes(d_ptr->ui.familyComboBox->currentText());
    int idx = 0;
    int i = 0;
    for (; i<sizeLst.count(); ++i) {
        if (idx == 0 && sizeLst.at(i) >= oldSize)
            idx = i;
        d_ptr->ui.sizeComboBox->addItem(QString::number(sizeLst.at(i)));
    }
    if (d_ptr->ui.sizeComboBox->count())
        d_ptr->ui.sizeComboBox->setCurrentIndex(idx);
    updatePreview();
}

void FontSettingsPage::delayedChange()
{
    emit changed(d_ptr->m_value);
}

void FontSettingsPage::apply()
{
    d_ptr->m_value.setFamily(d_ptr->ui.familyComboBox->currentText());

    bool ok = true;
    const int size = d_ptr->ui.sizeComboBox->currentText().toInt(&ok);
    if (ok)
        d_ptr->m_value.setFontSize(size);


    if (d_ptr->m_value != d_ptr->m_lastValue) {
        d_ptr->m_lastValue = d_ptr->m_value;
        if (QSettings *settings = Core::ICore::instance()->settings())
            d_ptr->m_value.toSettings(d_ptr->m_settingsGroup, d_ptr->m_descriptions, settings);

        QTimer::singleShot(0, this, SLOT(delayedChange()));
    }
}

void FontSettingsPage::finish()
{
    // If changes were applied, these are equal. Otherwise restores last value.
    d_ptr->m_value = d_ptr->m_lastValue;
}

const FontSettings &FontSettingsPage::fontSettings() const
{
    return d_ptr->m_value;
}
