// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "behaviorsettingswidget.h"

#include "behaviorsettings.h"
#include "codecchooser.h"
#include "extraencodingsettings.h"
#include "simplecodestylepreferenceswidget.h"
#include "storagesettings.h"
#include "tabsettingswidget.h"
#include "typingsettings.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QTextCodec>

#include <functional>

namespace TextEditor {

struct BehaviorSettingsWidgetPrivate
{

    SimpleCodeStylePreferencesWidget *tabPreferencesWidget;
    QComboBox *tabKeyBehavior;
    QComboBox *smartBackspaceBehavior;
    QCheckBox *autoIndent;
    QCheckBox *preferSingleLineComments;
    QGroupBox *groupBoxStorageSettings;
    QGroupBox *groupBoxTyping;
    QCheckBox *skipTrailingWhitespace;
    QLineEdit *ignoreFileTypes;
    QCheckBox *addFinalNewLine;
    QCheckBox *cleanWhitespace;
    QCheckBox *cleanIndentation;
    QCheckBox *inEntireDocument;
    QGroupBox *groupBoxEncodings;
    CodecChooser *encodingBox;
    QComboBox *utf8BomBox;
    QLabel *defaultLineEndingsLabel;
    QComboBox *defaultLineEndings;
    QGroupBox *groupBoxMouse;
    QCheckBox *mouseHiding;
    QCheckBox *mouseNavigation;
    QCheckBox *scrollWheelZooming;
    QCheckBox *camelCaseNavigation;
    QCheckBox *smartSelectionChanging;
    QCheckBox *keyboardTooltips;
    QComboBox *constrainTooltipsBox;
};

BehaviorSettingsWidget::BehaviorSettingsWidget(QWidget *parent)
    : QWidget(parent)
    , d(new BehaviorSettingsWidgetPrivate)
{
    resize(801, 693);

    d->tabPreferencesWidget = new SimpleCodeStylePreferencesWidget(this);
    d->tabPreferencesWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed); // FIXME: Desirable?

    d->tabKeyBehavior = new QComboBox;
    d->tabKeyBehavior->addItem(tr("Never"));
    d->tabKeyBehavior->addItem(tr("Always"));
    d->tabKeyBehavior->addItem(tr("In Leading White Space"));

    d->smartBackspaceBehavior = new QComboBox;
    d->smartBackspaceBehavior->addItem(tr("None"));
    d->smartBackspaceBehavior->addItem(tr("Follows Previous Indents"));
    d->smartBackspaceBehavior->addItem(tr("Unindents"));
    d->smartBackspaceBehavior->setToolTip(tr("<html><head/><body>\n"
"Specifies how backspace interacts with indentation.\n"
"\n"
"<ul>\n"
"<li>None: No interaction at all. Regular plain backspace behavior.\n"
"</li>\n"
"\n"
"<li>Follows Previous Indents: In leading white space it will take the cursor back to the nearest indentation level used in previous lines.\n"
"</li>\n"
"\n"
"<li>Unindents: If the character behind the cursor is a space it behaves as a backtab.\n"
"</li>\n"
"</ul></body></html>\n"
""));

    d->autoIndent = new QCheckBox(tr("Enable automatic &indentation"));

    d->preferSingleLineComments = new QCheckBox(tr("Prefer single line comments"));

    d->skipTrailingWhitespace = new QCheckBox(tr("Skip clean whitespace for file types:"));
    d->skipTrailingWhitespace->setToolTip(tr("For the file patterns listed, do not trim trailing whitespace."));
    d->skipTrailingWhitespace->setEnabled(false);
    d->skipTrailingWhitespace->setChecked(false);

    d->ignoreFileTypes = new QLineEdit;
    d->ignoreFileTypes->setEnabled(false);
    d->ignoreFileTypes->setAcceptDrops(false);
    d->ignoreFileTypes->setToolTip(tr("List of wildcard-aware file patterns, separated by commas or semicolons."));

    d->addFinalNewLine = new QCheckBox(tr("&Ensure newline at end of file"));
    d->addFinalNewLine->setToolTip(tr("Always writes a newline character at the end of the file."));

    d->cleanWhitespace = new QCheckBox(tr("&Clean whitespace"));
    d->cleanWhitespace->setToolTip(tr("Removes trailing whitespace upon saving."));

    d->cleanIndentation = new QCheckBox(tr("Clean indentation"));
    d->cleanIndentation->setEnabled(false);
    d->cleanIndentation->setToolTip(tr("Corrects leading whitespace according to tab settings."));

    d->inEntireDocument = new QCheckBox(tr("In entire &document"));
    d->inEntireDocument->setEnabled(false);
    d->inEntireDocument->setToolTip(tr("Cleans whitespace in entire document instead of only for changed parts."));

    d->encodingBox = new CodecChooser;
    d->encodingBox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    d->encodingBox->setMinimumContentsLength(20);

    d->utf8BomBox = new QComboBox;
    d->utf8BomBox->addItem(tr("Add If Encoding Is UTF-8"));
    d->utf8BomBox->addItem(tr("Keep If Already Present"));
    d->utf8BomBox->addItem(tr("Always Delete"));
    d->utf8BomBox->setToolTip(tr("<html><head/><body>\n"
"<p>How text editors should deal with UTF-8 Byte Order Marks. The options are:</p>\n"
"<ul ><li><i>Add If Encoding Is UTF-8:</i> always add a BOM when saving a file in UTF-8 encoding. Note that this will not work if the encoding is <i>System</i>, as the text editor does not know what it actually is.</li>\n"
"<li><i>Keep If Already Present: </i>save the file with a BOM if it already had one when it was loaded.</li>\n"
"<li><i>Always Delete:</i> never write an UTF-8 BOM, possibly deleting a pre-existing one.</li></ul>\n"
"<p>Note that UTF-8 BOMs are uncommon and treated incorrectly by some editors, so it usually makes little sense to add any.</p>\n"
"<p>This setting does <b>not</b> influence the use of UTF-16 and UTF-32 BOMs.</p></body></html>"));

    d->defaultLineEndings = new QComboBox;
    d->defaultLineEndings->addItems(ExtraEncodingSettings::lineTerminationModeNames());

    d->mouseHiding = new QCheckBox(tr("Hide mouse cursor while typing"));
    d->mouseNavigation = new QCheckBox(tr("Enable &mouse navigation"));
    d->scrollWheelZooming = new QCheckBox(tr("Enable scroll &wheel zooming"));
    d->camelCaseNavigation = new QCheckBox(tr("Enable built-in camel case &navigation"));

    d->smartSelectionChanging = new QCheckBox(tr("Enable smart selection changing"));
    d->smartSelectionChanging->setToolTip(tr("Using Select Block Up / Down actions will now provide smarter selections."));

    d->keyboardTooltips = new QCheckBox(tr("Show help tooltips using keyboard shortcut (Alt)"));
    d->keyboardTooltips->setToolTip(tr("Pressing Alt displays context-sensitive help or type information as tooltips."));

    d->constrainTooltipsBox = new QComboBox;
    d->constrainTooltipsBox->addItem(tr("On Mouseover"));
    d->constrainTooltipsBox->addItem(tr("On Shift+Mouseover"));

    d->groupBoxTyping = new QGroupBox(tr("Typing"));

    d->groupBoxStorageSettings = new QGroupBox(tr("Cleanups Upon Saving"));
    d->groupBoxStorageSettings->setToolTip(tr("Cleanup actions which are automatically performed "
                                              "right before the file is saved to disk."));
    d->groupBoxEncodings = new QGroupBox(tr("File Encodings"));

    d->groupBoxMouse = new QGroupBox(tr("Mouse and Keyboard"));

    using namespace Utils::Layouting;

    const auto indent = [](QWidget *inner) { return Row { Space(30), inner }; };

    Column {
        d->autoIndent,
        tr("Backspace indentation:"),
            indent(d->smartBackspaceBehavior),
        tr("Tab key performs auto-indent:"),
            indent(d->tabKeyBehavior),
        d->preferSingleLineComments
    }.attachTo(d->groupBoxTyping);

    Column {
        d->cleanWhitespace,
            indent(d->inEntireDocument),
            indent(d->cleanIndentation),
            Row { Space(30), d->skipTrailingWhitespace, d->ignoreFileTypes },
        d->addFinalNewLine,
    }.attachTo(d->groupBoxStorageSettings);

    Row {
        Form {
            tr("Default encoding:"), d->encodingBox, br,
            tr("UTF-8 BOM:"), d->utf8BomBox, br,
            tr("Default line endings:"), d->defaultLineEndings, br,
        }, st
    }.attachTo(d->groupBoxEncodings);

    Column {
        d->mouseHiding,
        d->mouseNavigation,
        d->scrollWheelZooming,
        d->camelCaseNavigation,
        d->smartSelectionChanging,
        d->keyboardTooltips,
        tr("Show help tooltips using the mouse:"),
            Row { Space(30), d->constrainTooltipsBox, st }
    }.attachTo(d->groupBoxMouse);

    Row {
        Column { d->tabPreferencesWidget, d->groupBoxTyping, st },
        Column { d->groupBoxStorageSettings, d->groupBoxEncodings, d->groupBoxMouse, st }
    }.attachTo(this, WithoutMargins);

    connect(d->cleanWhitespace, &QCheckBox::toggled,
            d->inEntireDocument, &QCheckBox::setEnabled);
    connect(d->cleanWhitespace, &QCheckBox::toggled,
            d->cleanIndentation, &QCheckBox::setEnabled);
    connect(d->cleanWhitespace, &QCheckBox::toggled,
            d->skipTrailingWhitespace, &QCheckBox::setEnabled);
    connect(d->cleanWhitespace, &QCheckBox::toggled,
            d->ignoreFileTypes, &QLineEdit::setEnabled);
    connect(d->autoIndent, &QAbstractButton::toggled,
            this, &BehaviorSettingsWidget::slotTypingSettingsChanged);
    connect(d->smartBackspaceBehavior, &QComboBox::currentIndexChanged,
            this, &BehaviorSettingsWidget::slotTypingSettingsChanged);
    connect(d->tabKeyBehavior, &QComboBox::currentIndexChanged,
            this, &BehaviorSettingsWidget::slotTypingSettingsChanged);
    connect(d->cleanWhitespace, &QAbstractButton::clicked,
            this, &BehaviorSettingsWidget::slotStorageSettingsChanged);
    connect(d->inEntireDocument, &QAbstractButton::clicked,
            this, &BehaviorSettingsWidget::slotStorageSettingsChanged);
    connect(d->addFinalNewLine, &QAbstractButton::clicked,
            this, &BehaviorSettingsWidget::slotStorageSettingsChanged);
    connect(d->cleanIndentation, &QAbstractButton::clicked,
            this, &BehaviorSettingsWidget::slotStorageSettingsChanged);
    connect(d->skipTrailingWhitespace, &QAbstractButton::clicked,
            this, &BehaviorSettingsWidget::slotStorageSettingsChanged);
    connect(d->mouseHiding, &QAbstractButton::clicked,
            this, &BehaviorSettingsWidget::slotBehaviorSettingsChanged);
    connect(d->mouseNavigation, &QAbstractButton::clicked,
            this, &BehaviorSettingsWidget::slotBehaviorSettingsChanged);
    connect(d->scrollWheelZooming, &QAbstractButton::clicked,
            this, &BehaviorSettingsWidget::slotBehaviorSettingsChanged);
    connect(d->camelCaseNavigation, &QAbstractButton::clicked,
            this, &BehaviorSettingsWidget::slotBehaviorSettingsChanged);
    connect(d->utf8BomBox, &QComboBox::currentIndexChanged,
            this, &BehaviorSettingsWidget::slotExtraEncodingChanged);
    connect(d->encodingBox, &CodecChooser::codecChanged,
            this, &BehaviorSettingsWidget::textCodecChanged);
    connect(d->constrainTooltipsBox, &QComboBox::currentIndexChanged,
            this, &BehaviorSettingsWidget::slotBehaviorSettingsChanged);
    connect(d->keyboardTooltips, &QAbstractButton::clicked,
            this, &BehaviorSettingsWidget::slotBehaviorSettingsChanged);
    connect(d->smartSelectionChanging, &QAbstractButton::clicked,
            this, &BehaviorSettingsWidget::slotBehaviorSettingsChanged);

    d->mouseHiding->setVisible(!Utils::HostOsInfo::isMacHost());
}

BehaviorSettingsWidget::~BehaviorSettingsWidget()
{
    delete d;
}

void BehaviorSettingsWidget::setActive(bool active)
{
    d->tabPreferencesWidget->setEnabled(active);
    d->groupBoxTyping->setEnabled(active);
    d->groupBoxEncodings->setEnabled(active);
    d->groupBoxMouse->setEnabled(active);
    d->groupBoxStorageSettings->setEnabled(active);
}

void BehaviorSettingsWidget::setAssignedCodec(QTextCodec *codec)
{
    const QString codecName = Core::ICore::settings()->value(
                Core::Constants::SETTINGS_DEFAULTTEXTENCODING).toString();
    d->encodingBox->setAssignedCodec(codec, codecName);
}

QByteArray BehaviorSettingsWidget::assignedCodecName() const
{
    return d->encodingBox->assignedCodecName();
}

void BehaviorSettingsWidget::setCodeStyle(ICodeStylePreferences *preferences)
{
    d->tabPreferencesWidget->setPreferences(preferences);
}

void BehaviorSettingsWidget::setAssignedTypingSettings(const TypingSettings &typingSettings)
{
    d->autoIndent->setChecked(typingSettings.m_autoIndent);
    d->smartBackspaceBehavior->setCurrentIndex(typingSettings.m_smartBackspaceBehavior);
    d->tabKeyBehavior->setCurrentIndex(typingSettings.m_tabKeyBehavior);

    d->preferSingleLineComments->setChecked(typingSettings.m_preferSingleLineComments);
}

void BehaviorSettingsWidget::assignedTypingSettings(TypingSettings *typingSettings) const
{
    typingSettings->m_autoIndent = d->autoIndent->isChecked();
    typingSettings->m_smartBackspaceBehavior =
        (TypingSettings::SmartBackspaceBehavior)(d->smartBackspaceBehavior->currentIndex());
    typingSettings->m_tabKeyBehavior =
        (TypingSettings::TabKeyBehavior)(d->tabKeyBehavior->currentIndex());

    typingSettings->m_preferSingleLineComments = d->preferSingleLineComments->isChecked();
}

void BehaviorSettingsWidget::setAssignedStorageSettings(const StorageSettings &storageSettings)
{
    d->cleanWhitespace->setChecked(storageSettings.m_cleanWhitespace);
    d->inEntireDocument->setChecked(storageSettings.m_inEntireDocument);
    d->cleanIndentation->setChecked(storageSettings.m_cleanIndentation);
    d->addFinalNewLine->setChecked(storageSettings.m_addFinalNewLine);
    d->skipTrailingWhitespace->setChecked(storageSettings.m_skipTrailingWhitespace);
    d->ignoreFileTypes->setText(storageSettings.m_ignoreFileTypes);
    d->ignoreFileTypes->setEnabled(d->skipTrailingWhitespace->isChecked());
}

void BehaviorSettingsWidget::assignedStorageSettings(StorageSettings *storageSettings) const
{
    storageSettings->m_cleanWhitespace = d->cleanWhitespace->isChecked();
    storageSettings->m_inEntireDocument = d->inEntireDocument->isChecked();
    storageSettings->m_cleanIndentation = d->cleanIndentation->isChecked();
    storageSettings->m_addFinalNewLine = d->addFinalNewLine->isChecked();
    storageSettings->m_skipTrailingWhitespace = d->skipTrailingWhitespace->isChecked();
    storageSettings->m_ignoreFileTypes = d->ignoreFileTypes->text();
}

void BehaviorSettingsWidget::updateConstrainTooltipsBoxTooltip() const
{
    if (d->constrainTooltipsBox->currentIndex() == 0) {
        d->constrainTooltipsBox->setToolTip(
            tr("Displays context-sensitive help or type information on mouseover."));
    } else {
        d->constrainTooltipsBox->setToolTip(
            tr("Displays context-sensitive help or type information on Shift+Mouseover."));
    }
}

void BehaviorSettingsWidget::setAssignedBehaviorSettings(const BehaviorSettings &behaviorSettings)
{
    d->mouseHiding->setChecked(behaviorSettings.m_mouseHiding);
    d->mouseNavigation->setChecked(behaviorSettings.m_mouseNavigation);
    d->scrollWheelZooming->setChecked(behaviorSettings.m_scrollWheelZooming);
    d->constrainTooltipsBox->setCurrentIndex(behaviorSettings.m_constrainHoverTooltips ? 1 : 0);
    d->camelCaseNavigation->setChecked(behaviorSettings.m_camelCaseNavigation);
    d->keyboardTooltips->setChecked(behaviorSettings.m_keyboardTooltips);
    d->smartSelectionChanging->setChecked(behaviorSettings.m_smartSelectionChanging);
    updateConstrainTooltipsBoxTooltip();
}

void BehaviorSettingsWidget::assignedBehaviorSettings(BehaviorSettings *behaviorSettings) const
{
    behaviorSettings->m_mouseHiding = d->mouseHiding->isChecked();
    behaviorSettings->m_mouseNavigation = d->mouseNavigation->isChecked();
    behaviorSettings->m_scrollWheelZooming = d->scrollWheelZooming->isChecked();
    behaviorSettings->m_constrainHoverTooltips = (d->constrainTooltipsBox->currentIndex() == 1);
    behaviorSettings->m_camelCaseNavigation = d->camelCaseNavigation->isChecked();
    behaviorSettings->m_keyboardTooltips = d->keyboardTooltips->isChecked();
    behaviorSettings->m_smartSelectionChanging = d->smartSelectionChanging->isChecked();
}

void BehaviorSettingsWidget::setAssignedExtraEncodingSettings(
    const ExtraEncodingSettings &encodingSettings)
{
    d->utf8BomBox->setCurrentIndex(encodingSettings.m_utf8BomSetting);
}

void BehaviorSettingsWidget::assignedExtraEncodingSettings(
    ExtraEncodingSettings *encodingSettings) const
{
    encodingSettings->m_utf8BomSetting =
        (ExtraEncodingSettings::Utf8BomSetting)d->utf8BomBox->currentIndex();
}

void BehaviorSettingsWidget::setAssignedLineEnding(int lineEnding)
{
    d->defaultLineEndings->setCurrentIndex(lineEnding);
}

int BehaviorSettingsWidget::assignedLineEnding() const
{
    return d->defaultLineEndings->currentIndex();
}

TabSettingsWidget *BehaviorSettingsWidget::tabSettingsWidget() const
{
    return d->tabPreferencesWidget->tabSettingsWidget();
}

void BehaviorSettingsWidget::slotTypingSettingsChanged()
{
    TypingSettings settings;
    assignedTypingSettings(&settings);
    emit typingSettingsChanged(settings);
}

void BehaviorSettingsWidget::slotStorageSettingsChanged()
{
    StorageSettings settings;
    assignedStorageSettings(&settings);

    bool ignoreFileTypesEnabled = d->cleanWhitespace->isChecked() && d->skipTrailingWhitespace->isChecked();
    d->ignoreFileTypes->setEnabled(ignoreFileTypesEnabled);

    emit storageSettingsChanged(settings);
}

void BehaviorSettingsWidget::slotBehaviorSettingsChanged()
{
    BehaviorSettings settings;
    assignedBehaviorSettings(&settings);
    updateConstrainTooltipsBoxTooltip();
    emit behaviorSettingsChanged(settings);
}

void BehaviorSettingsWidget::slotExtraEncodingChanged()
{
    ExtraEncodingSettings settings;
    assignedExtraEncodingSettings(&settings);
    emit extraEncodingSettingsChanged(settings);
}

} // TextEditor
