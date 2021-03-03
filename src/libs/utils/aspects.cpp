/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "aspects.h"

#include "algorithm.h"
#include "fancylineedit.h"
#include "layoutbuilder.h"
#include "pathchooser.h"
#include "qtcassert.h"
#include "qtcprocess.h"
#include "utilsicons.h"
#include "variablechooser.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPointer>
#include <QRadioButton>
#include <QSpinBox>
#include <QTextEdit>
#include <QToolButton>

namespace Utils {
namespace Internal {

class BaseAspectPrivate
{
public:
    Utils::Id m_id;
    QVariant m_value;
    QVariant m_defaultValue;

    QString m_displayName;
    QString m_settingsKey; // Name of data in settings.
    QString m_tooltip;
    QString m_labelText;
    QPixmap m_labelPixmap;
    QPointer<QLabel> m_label; // Owned by configuration widget

    bool m_visible = true;
    bool m_enabled = true;
    bool m_readOnly = true;
    BaseAspect::ConfigWidgetCreator m_configWidgetCreator;
    QList<QPointer<QWidget>> m_subWidgets;
};

} // Internal

/*!
    \class Utils::BaseAspect
    \inmodule QtCreator

    \brief The \c BaseAspect class provides a common base for classes implementing
    aspects.

    An aspect is a hunk of data like a property or collection of related
    properties of some object, together with a description of its behavior
    for common operations like visualizing or persisting.

    Simple aspects are for example a boolean property represented by a QCheckBox
    in the user interface, or a string property represented by a PathChooser,
    selecting directories in the filesystem.

    While aspects implementations usually have the ability to visualize and to persist
    their data, or use an ID, neither of these is mandatory.
*/

/*!
    Constructs a BaseAspect.
*/
BaseAspect::BaseAspect()
    : d(new Internal::BaseAspectPrivate)
{}

/*!
    Destructs a BaseAspect.
*/
BaseAspect::~BaseAspect() = default;

Id BaseAspect::id() const
{
    return d->m_id;
}

void BaseAspect::setId(Id id)
{
    d->m_id = id;
}

QVariant BaseAspect::value() const
{
    return d->m_value;
}

/*!
    Sets value.

    Emits changed() if the value changed.
*/
void BaseAspect::setValue(const QVariant &value)
{
    if (setValueQuietly(value))
        emit changed();
}

/*!
    Sets value without emitting changed()

    Returns whether the value changed.
*/
bool BaseAspect::setValueQuietly(const QVariant &value)
{
    if (d->m_value == value)
        return false;
    d->m_value = value;
    return true;
}

QVariant BaseAspect::defaultValue() const
{
    return d->m_defaultValue;
}

/*!
    Sets a default value for this aspect.

    Default values will not be stored in settings.
*/
void BaseAspect::setDefaultValue(const QVariant &value)
{
    d->m_defaultValue = value;
}

void BaseAspect::setDisplayName(const QString &displayName)
{
    d->m_displayName = displayName;
}

bool BaseAspect::isVisible() const
{
    return d->m_visible;
}

/*!
    Shows or hides the visual representation of this aspect depending
    on the value of \a visible.
    By default, it is visible.
 */
void BaseAspect::setVisible(bool visible)
{
    d->m_visible = visible;
    for (QWidget *w : qAsConst(d->m_subWidgets)) {
        QTC_ASSERT(w, continue);
        w->setVisible(visible);
    }
}

void BaseAspect::setupLabel()
{
    QTC_ASSERT(!d->m_label, delete d->m_label);
    d->m_label = new QLabel(d->m_labelText);
    d->m_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    if (!d->m_labelPixmap.isNull())
        d->m_label->setPixmap(d->m_labelPixmap);
    registerSubWidget(d->m_label);
}

/*!
    Sets \a labelText as text for the separate label in the visual
    representation of this aspect.
*/
void BaseAspect::setLabelText(const QString &labelText)
{
    d->m_labelText = labelText;
    if (d->m_label)
        d->m_label->setText(labelText);
}

/*!
    Sets \a labelPixmap as pixmap for the separate label in the visual
    representation of this aspect.
*/
void BaseAspect::setLabelPixmap(const QPixmap &labelPixmap)
{
    d->m_labelPixmap = labelPixmap;
    if (d->m_label)
        d->m_label->setPixmap(labelPixmap);
}

/*!
    Returns the current text for the separate label in the visual
    representation of this aspect.
*/
QString BaseAspect::labelText() const
{
    return d->m_labelText;
}

QLabel *BaseAspect::label() const
{
    return d->m_label.data();
}

QString BaseAspect::toolTip() const
{
    return d->m_tooltip;
}

/*!
    Sets \a tooltip as tool tip for the visual representation of this aspect.
 */
void BaseAspect::setToolTip(const QString &tooltip)
{
    d->m_tooltip = tooltip;
    for (QWidget *w : qAsConst(d->m_subWidgets)) {
        QTC_ASSERT(w, continue);
        w->setToolTip(tooltip);
    }
}

void BaseAspect::setEnabled(bool enabled)
{
    d->m_enabled = enabled;
    for (QWidget *w : qAsConst(d->m_subWidgets)) {
        QTC_ASSERT(w, continue);
        w->setEnabled(enabled);
    }
}

void BaseAspect::setReadOnly(bool readOnly)
{
    d->m_readOnly = readOnly;
    for (QWidget *w : qAsConst(d->m_subWidgets)) {
        QTC_ASSERT(w, continue);
        if (auto lineEdit = qobject_cast<QLineEdit *>(w))
            lineEdit->setReadOnly(readOnly);
        else if (auto textEdit = qobject_cast<QTextEdit *>(w))
            textEdit->setReadOnly(readOnly);
    }
}

/*!
    \internal
*/
void BaseAspect::setConfigWidgetCreator(const ConfigWidgetCreator &configWidgetCreator)
{
    d->m_configWidgetCreator = configWidgetCreator;
}

/*!
    Returns the key to be used when accessing the settings.

    \sa setSettingsKey()
*/
QString BaseAspect::settingsKey() const
{
    return d->m_settingsKey;
}

/*!
    Sets the key to be used when accessing the settings.

    \sa settingsKey()
*/
void BaseAspect::setSettingsKey(const QString &key)
{
    d->m_settingsKey = key;
}

/*!
    Sets the key and group to be used when accessing the settings.

    \sa settingsKey()
*/
void BaseAspect::setSettingsKey(const QString &group, const QString &key)
{
    d->m_settingsKey = group + "/" + key;
}

QString BaseAspect::displayName() const { return d->m_displayName; }

/*!
    \internal
*/
QWidget *BaseAspect::createConfigWidget() const
{
    return d->m_configWidgetCreator ? d->m_configWidgetCreator() : nullptr;
}

/*!
    Adds the visual representation of this aspect to a layout using
    a layout builder.
*/
void BaseAspect::addToLayout(LayoutBuilder &)
{
}

void BaseAspect::registerSubWidget(QWidget *widget)
{
    d->m_subWidgets.append(widget);

    connect(widget, &QObject::destroyed, this, [this, widget] {
        d->m_subWidgets.removeAll(widget);
    });

    widget->setEnabled(d->m_enabled);
    widget->setToolTip(d->m_tooltip);

    // Visible is on by default. Not setting it explicitly avoid popping
    // it up when the parent is not set yet, the normal case.
    if (!d->m_visible)
        widget->setVisible(d->m_visible);
}

void BaseAspect::saveToMap(QVariantMap &data, const QVariant &value,
                           const QVariant &defaultValue, const QString &keyExtension) const
{
    if (settingsKey().isEmpty())
        return;
    const QString key = settingsKey() + keyExtension;
    if (value == defaultValue)
        data.remove(key);
    else
        data.insert(key, value);
}

/*!
    Retrieves the internal value of this BaseAspect from a \c QVariantMap.
*/
void BaseAspect::fromMap(const QVariantMap &map)
{
    setValue(map.value(settingsKey(), defaultValue()));
}

/*!
    Stores the internal value of this BaseAspect into a \c QVariantMap.
*/
void BaseAspect::toMap(QVariantMap &map) const
{
    saveToMap(map, d->m_value, d->m_defaultValue);
}

/*!
    \internal
*/
void BaseAspect::acquaintSiblings(const BaseAspects &)
{}

// BaseAspects

/*!
    \class BaseAspects
    \inmodule QtCreator

    \brief This class represent a collection of one or more aspects.

    A BaseAspects object assumes ownership on its aspects.
*/

/*!
    Constructs a BaseAspects object.
*/
BaseAspects::BaseAspects() = default;

/*!
    Destructs a BaseAspects object.
*/
BaseAspects::~BaseAspects()
{
    qDeleteAll(m_aspects);
}

/*!
    Retrieves a BaseAspect with a given \a id, or nullptr if no such aspect is contained.

    \sa BaseAspect.
*/
BaseAspect *BaseAspects::aspect(Utils::Id id) const
{
    return Utils::findOrDefault(m_aspects, Utils::equal(&BaseAspect::id, id));
}

/*!
    \internal
*/
void BaseAspects::fromMap(const QVariantMap &map) const
{
    for (BaseAspect *aspect : m_aspects)
        aspect->fromMap(map);
}

/*!
    \internal
*/
void BaseAspects::toMap(QVariantMap &map) const
{
    for (BaseAspect *aspect : m_aspects)
        aspect->toMap(map);
}

namespace Internal {

class BoolAspectPrivate
{
public:
    BoolAspect::LabelPlacement m_labelPlacement = BoolAspect::LabelPlacement::AtCheckBox;
    QPointer<QCheckBox> m_checkBox; // Owned by configuration widget
};

class SelectionAspectPrivate
{
public:
    SelectionAspect::DisplayStyle m_displayStyle
            = SelectionAspect::DisplayStyle::RadioButtons;
    struct Option { QString displayName; QString tooltip; };
    QVector<Option> m_options;

    // These are all owned by the configuration widget.
    QList<QPointer<QRadioButton>> m_buttons;
    QPointer<QComboBox> m_comboBox;
    QPointer<QButtonGroup> m_buttonGroup;
};

class MultiSelectionAspectPrivate
{
public:
    explicit MultiSelectionAspectPrivate(MultiSelectionAspect *q) : q(q) {}

    bool setValueSelectedHelper(const QString &value, bool on);

    MultiSelectionAspect *q;
    QStringList m_allValues;
    MultiSelectionAspect::DisplayStyle m_displayStyle
        = MultiSelectionAspect::DisplayStyle::ListView;

    // These are all owned by the configuration widget.
    QPointer<QListWidget> m_listView;
};

class StringAspectPrivate
{
public:
    StringAspect::DisplayStyle m_displayStyle = StringAspect::LabelDisplay;
    StringAspect::CheckBoxPlacement m_checkBoxPlacement
        = StringAspect::CheckBoxPlacement::Right;
    StringAspect::UncheckedSemantics m_uncheckedSemantics
        = StringAspect::UncheckedSemantics::Disabled;
    std::function<QString(const QString &)> m_displayFilter;
    std::unique_ptr<BoolAspect> m_checker;

    QString m_placeHolderText;
    QString m_historyCompleterKey;
    PathChooser::Kind m_expectedKind = PathChooser::File;
    Environment m_environment;
    QPointer<QLabel> m_labelDisplay;
    QPointer<FancyLineEdit> m_lineEditDisplay;
    QPointer<PathChooser> m_pathChooserDisplay;
    QPointer<QTextEdit> m_textEditDisplay;
    MacroExpanderProvider m_expanderProvider;
    FilePath m_baseFileName;
    StringAspect::ValueAcceptor m_valueAcceptor;
    FancyLineEdit::ValidationFunction m_validator;
    std::function<void()> m_openTerminal;

    bool m_undoRedoEnabled = true;
    bool m_showToolTipOnLabel = false;
    bool m_fileDialogOnly = false;

    template<class Widget> void updateWidgetFromCheckStatus(Widget *w)
    {
        const bool enabled = !m_checker || m_checker->value();
        if (m_uncheckedSemantics == StringAspect::UncheckedSemantics::Disabled)
            w->setEnabled(enabled);
        else
            w->setReadOnly(!enabled);
    }
};

class IntegerAspectPrivate
{
public:
    QVariant m_minimumValue;
    QVariant m_maximumValue;
    int m_displayIntegerBase = 10;
    qint64 m_displayScaleFactor = 1;
    QString m_prefix;
    QString m_suffix;
    QPointer<QSpinBox> m_spinBox; // Owned by configuration widget
};

class StringListAspectPrivate
{
public:
};

class AspectContainerPrivate
{
public:
    QList<BaseAspect *> m_items;
};

class TextDisplayPrivate
{
public:
    QString m_message;
    Utils::InfoLabel::InfoType m_type;
    QPointer<InfoLabel> m_label;
};

} // Internal

/*!
    \enum Utils::StringAspect::DisplayStyle
    \inmodule QtCreator

    The DisplayStyle enum describes the main visual characteristics of a
    string aspect.

      \value LabelDisplay
             Based on QLabel, used for text that cannot be changed by the
             user in this place, for example names of executables that are
             defined in the build system.

      \value LineEditDisplay
             Based on QLineEdit, used for user-editable strings that usually
             fit on a line.

      \value TextEditDisplay
             Based on QTextEdit, used for user-editable strings that often
             do not fit on a line.

      \value PathChooserDisplay
             Based on Utils::PathChooser.

    \sa Utils::PathChooser
*/

/*!
    \class Utils::StringAspect
    \inmodule QtCreator

    \brief A string aspect is a string-like property of some object, together with
    a description of its behavior for common operations like visualizing or
    persisting.

    String aspects can represent for example a parameter for an external commands,
    paths in a file system, or simply strings.

    The string can be displayed using a QLabel, QLineEdit, QTextEdit or
    Utils::PathChooser.

    The visual representation often contains a label in front of the display
    of the actual value.
*/

/*!
    Constructs a StringAspect.
 */

StringAspect::StringAspect()
    : d(new Internal::StringAspectPrivate)
{}

/*!
    \internal
*/
StringAspect::~StringAspect() = default;

/*!
    \internal
*/
void StringAspect::setValueAcceptor(StringAspect::ValueAcceptor &&acceptor)
{
    d->m_valueAcceptor = std::move(acceptor);
}

/*!
    Returns the value of this StringAspect as an ordinary \c QString.
*/
QString StringAspect::value() const
{
    return BaseAspect::value().toString();
}

/*!
    Sets the \a value of this StringAspect from an ordinary \c QString.
*/
void StringAspect::setValue(const QString &val)
{
    const bool isSame = val == value();
    if (isSame)
        return;

    QString processedValue = val;
    if (d->m_valueAcceptor) {
        const Utils::optional<QString> tmp = d->m_valueAcceptor(value(), val);
        if (!tmp) {
            update(); // Make sure the original value is retained in the UI
            return;
        }
        processedValue = tmp.value();
    }

    if (BaseAspect::setValueQuietly(QVariant(processedValue))) {
        update();
        emit changed();
    }
}

/*!
    \reimp
*/
void StringAspect::fromMap(const QVariantMap &map)
{
    if (!settingsKey().isEmpty())
        BaseAspect::setValueQuietly(map.value(settingsKey()));
    if (d->m_checker)
        d->m_checker->fromMap(map);
}

/*!
    \reimp
*/
void StringAspect::toMap(QVariantMap &map) const
{
    saveToMap(map, value(), QString());
    if (d->m_checker)
        d->m_checker->toMap(map);
}

/*!
    Returns the value of this string aspect as \c Utils::FilePath.

    \note This simply uses \c FilePath::fromUserInput() for the
    conversion. It does not use any check that the value is actually
    a valid file path.
*/
FilePath StringAspect::filePath() const
{
    return FilePath::fromUserInput(value());
}

/*!
    Sets the value of this string aspect to \a value.

    \note This simply uses \c FilePath::toUserOutput() for the
    conversion. It does not use any check that the value is actually
    a file path.
*/
void StringAspect::setFilePath(const FilePath &value)
{
    setValue(value.toUserOutput());
}

/*!
    \internal
*/
void StringAspect::setShowToolTipOnLabel(bool show)
{
    d->m_showToolTipOnLabel = show;
    update();
}

/*!
    Sets a \a displayFilter for fine-tuning the visual appearance
    of the value of this string aspect.
*/
void StringAspect::setDisplayFilter(const std::function<QString(const QString &)> &displayFilter)
{
    d->m_displayFilter = displayFilter;
}

/*!
    Returns the check box value.

    \sa makeCheckable(), setChecked()
*/
bool StringAspect::isChecked() const
{
    return !d->m_checker || d->m_checker->value();
}

/*!
    Sets the check box of this aspect to \a checked.

    \sa makeCheckable(), isChecked()
*/
void StringAspect::setChecked(bool checked)
{
    QTC_ASSERT(d->m_checker, return);
    d->m_checker->setValue(checked);
}

/*!
    Selects the main display characteristics of the aspect according to
    \a displayStyle.

    \note Not all StringAspect features are available with all display styles.

    \sa Utils::StringAspect::DisplayStyle
*/
void StringAspect::setDisplayStyle(DisplayStyle displayStyle)
{
    d->m_displayStyle = displayStyle;
}

/*!
    Sets \a placeHolderText as place holder for line and text displays.
*/
void StringAspect::setPlaceHolderText(const QString &placeHolderText)
{
    d->m_placeHolderText = placeHolderText;
    if (d->m_lineEditDisplay)
        d->m_lineEditDisplay->setPlaceholderText(placeHolderText);
    if (d->m_textEditDisplay)
        d->m_textEditDisplay->setPlaceholderText(placeHolderText);
}

/*!
    Sets \a historyCompleterKey as key for the history completer settings for
    line edits and path chooser displays.

    \sa Utils::PathChooser::setExpectedKind()
*/
void StringAspect::setHistoryCompleter(const QString &historyCompleterKey)
{
    d->m_historyCompleterKey = historyCompleterKey;
    if (d->m_lineEditDisplay)
        d->m_lineEditDisplay->setHistoryCompleter(historyCompleterKey);
    if (d->m_pathChooserDisplay)
        d->m_pathChooserDisplay->setHistoryCompleter(historyCompleterKey);
}

/*!
  Sets \a expectedKind as expected kind for path chooser displays.

  \sa Utils::PathChooser::setExpectedKind()
*/
void StringAspect::setExpectedKind(const PathChooser::Kind expectedKind)
{
    d->m_expectedKind = expectedKind;
    if (d->m_pathChooserDisplay)
        d->m_pathChooserDisplay->setExpectedKind(expectedKind);
}

void StringAspect::setFileDialogOnly(bool requireFileDialog)
{
    d->m_fileDialogOnly = requireFileDialog;
    if (d->m_pathChooserDisplay)
        d->m_pathChooserDisplay->setFileDialogOnly(requireFileDialog);
}

void StringAspect::setEnvironment(const Environment &env)
{
    d->m_environment = env;
    if (d->m_pathChooserDisplay)
        d->m_pathChooserDisplay->setEnvironment(env);
}

void StringAspect::setBaseFileName(const FilePath &baseFileName)
{
    d->m_baseFileName = baseFileName;
    if (d->m_pathChooserDisplay)
        d->m_pathChooserDisplay->setBaseDirectory(baseFileName);
}

void StringAspect::setUndoRedoEnabled(bool undoRedoEnabled)
{
    d->m_undoRedoEnabled = undoRedoEnabled;
    if (d->m_textEditDisplay)
        d->m_textEditDisplay->setUndoRedoEnabled(undoRedoEnabled);
}

void StringAspect::setMacroExpanderProvider(const MacroExpanderProvider &expanderProvider)
{
    d->m_expanderProvider = expanderProvider;
}

void StringAspect::setValidationFunction(const FancyLineEdit::ValidationFunction &validator)
{
    d->m_validator = validator;
    if (d->m_lineEditDisplay)
        d->m_lineEditDisplay->setValidationFunction(d->m_validator);
}

void StringAspect::setOpenTerminalHandler(const std::function<void ()> &openTerminal)
{
    d->m_openTerminal = openTerminal;
    if (d->m_pathChooserDisplay)
        d->m_pathChooserDisplay->setOpenTerminalHandler(openTerminal);
}

void StringAspect::validateInput()
{
    if (d->m_pathChooserDisplay)
        d->m_pathChooserDisplay->triggerChanged();
    if (d->m_lineEditDisplay)
        d->m_lineEditDisplay->validate();
}

void StringAspect::setUncheckedSemantics(StringAspect::UncheckedSemantics semantics)
{
    d->m_uncheckedSemantics = semantics;
}

void StringAspect::addToLayout(LayoutBuilder &builder)
{
    if (d->m_checker && d->m_checkBoxPlacement == CheckBoxPlacement::Top) {
        d->m_checker->addToLayout(builder);
        builder.finishRow();
    }

    setupLabel();
    builder.addItem(label());

    const auto useMacroExpander = [this, &builder](QWidget *w) {
        if (!d->m_expanderProvider)
            return;
        const auto chooser = new VariableChooser(builder.layout()->parentWidget());
        chooser->addSupportedWidget(w);
        chooser->addMacroExpanderProvider(d->m_expanderProvider);
    };

    switch (d->m_displayStyle) {
    case PathChooserDisplay:
        d->m_pathChooserDisplay = createSubWidget<PathChooser>();
        d->m_pathChooserDisplay->setExpectedKind(d->m_expectedKind);
        if (!d->m_historyCompleterKey.isEmpty())
            d->m_pathChooserDisplay->setHistoryCompleter(d->m_historyCompleterKey);
        d->m_pathChooserDisplay->setEnvironment(d->m_environment);
        d->m_pathChooserDisplay->setBaseDirectory(d->m_baseFileName);
        useMacroExpander(d->m_pathChooserDisplay->lineEdit());
        connect(d->m_pathChooserDisplay, &PathChooser::pathChanged,
                this, &StringAspect::setValue);
        builder.addItem(d->m_pathChooserDisplay.data());
        d->m_pathChooserDisplay->setFileDialogOnly(d->m_fileDialogOnly);
        d->m_pathChooserDisplay->setOpenTerminalHandler(d->m_openTerminal);
        break;
    case LineEditDisplay:
        d->m_lineEditDisplay = createSubWidget<FancyLineEdit>();
        d->m_lineEditDisplay->setPlaceholderText(d->m_placeHolderText);
        if (!d->m_historyCompleterKey.isEmpty())
            d->m_lineEditDisplay->setHistoryCompleter(d->m_historyCompleterKey);
        if (d->m_validator)
            d->m_lineEditDisplay->setValidationFunction(d->m_validator);
        useMacroExpander(d->m_lineEditDisplay);
        connect(d->m_lineEditDisplay, &FancyLineEdit::textEdited,
                this, &StringAspect::setValue);
        builder.addItem(d->m_lineEditDisplay.data());
        break;
    case TextEditDisplay:
        d->m_textEditDisplay = createSubWidget<QTextEdit>();
        d->m_textEditDisplay->setPlaceholderText(d->m_placeHolderText);
        d->m_textEditDisplay->setUndoRedoEnabled(d->m_undoRedoEnabled);
        d->m_textEditDisplay->setAcceptRichText(false);
        d->m_textEditDisplay->setTextInteractionFlags(Qt::TextEditorInteraction);
        useMacroExpander(d->m_textEditDisplay);
        connect(d->m_textEditDisplay, &QTextEdit::textChanged, this, [this] {
            const QString value = d->m_textEditDisplay->document()->toPlainText();
            setValue(value);
        });
        builder.addItem(d->m_textEditDisplay.data());
        break;
    case LabelDisplay:
        d->m_labelDisplay = createSubWidget<QLabel>();
        d->m_labelDisplay->setTextInteractionFlags(Qt::TextSelectableByMouse);
        builder.addItem(d->m_labelDisplay.data());
        break;
    }

    validateInput();

    if (d->m_checker && d->m_checkBoxPlacement == CheckBoxPlacement::Right)
        d->m_checker->addToLayout(builder);

    update();
}

void StringAspect::update()
{
    const QString displayedString = d->m_displayFilter ? d->m_displayFilter(value()) : value();

    if (d->m_pathChooserDisplay) {
        d->m_pathChooserDisplay->setFilePath(FilePath::fromString(displayedString));
        d->updateWidgetFromCheckStatus(d->m_pathChooserDisplay.data());
    }

    if (d->m_lineEditDisplay) {
        d->m_lineEditDisplay->setTextKeepingActiveCursor(displayedString);
        d->updateWidgetFromCheckStatus(d->m_lineEditDisplay.data());
    }

    if (d->m_textEditDisplay) {
        const QString old = d->m_textEditDisplay->document()->toPlainText();
        if (displayedString != old)
            d->m_textEditDisplay->setText(displayedString);
        d->updateWidgetFromCheckStatus(d->m_textEditDisplay.data());
    }

    if (d->m_labelDisplay) {
        d->m_labelDisplay->setText(displayedString);
        d->m_labelDisplay->setToolTip(d->m_showToolTipOnLabel ? displayedString : toolTip());
    }

    validateInput();
}

/*!
    Adds a check box with a \a checkerLabel according to \a checkBoxPlacement
    to the line edit.

    The state of the check box is made persistent when using a non-emtpy
    \a checkerKey.
*/
void StringAspect::makeCheckable(CheckBoxPlacement checkBoxPlacement,
                                     const QString &checkerLabel, const QString &checkerKey)
{
    QTC_ASSERT(!d->m_checker, return);
    d->m_checkBoxPlacement = checkBoxPlacement;
    d->m_checker.reset(new BoolAspect);
    d->m_checker->setLabel(checkerLabel, checkBoxPlacement == CheckBoxPlacement::Top
                           ? BoolAspect::LabelPlacement::InExtraLabel
                           : BoolAspect::LabelPlacement::AtCheckBox);
    d->m_checker->setSettingsKey(checkerKey);

    connect(d->m_checker.get(), &BoolAspect::changed, this, &StringAspect::update);
    connect(d->m_checker.get(), &BoolAspect::changed, this, &StringAspect::changed);
    connect(d->m_checker.get(), &BoolAspect::changed, this, &StringAspect::checkedChanged);

    update();
}

/*!
    \class Utils::BoolAspect
    \inmodule QtCreator

    \brief A boolean aspect is a boolean property of some object, together with
    a description of its behavior for common operations like visualizing or
    persisting.

    The boolean aspect is displayed using a QCheckBox.

    The visual representation often contains a label in front or after
    the display of the actual checkmark.
*/


BoolAspect::BoolAspect(const QString &settingsKey)
    : d(new Internal::BoolAspectPrivate)
{
    setValue(false);
    setDefaultValue(false);
    setSettingsKey(settingsKey);
}

/*!
    \reimp
*/
BoolAspect::~BoolAspect() = default;

/*!
    \reimp
*/
void BoolAspect::addToLayout(LayoutBuilder &builder)
{
    QTC_CHECK(!d->m_checkBox);
    d->m_checkBox = createSubWidget<QCheckBox>();
    switch (d->m_labelPlacement) {
    case LabelPlacement::AtCheckBoxWithoutDummyLabel:
        d->m_checkBox->setText(labelText());
        break;
    case LabelPlacement::AtCheckBox:
        d->m_checkBox->setText(labelText());
        builder.addItem(createSubWidget<QLabel>());
        break;
    case LabelPlacement::InExtraLabel:
        setupLabel();
        builder.addItem(label());
        break;
    }
    d->m_checkBox->setChecked(value());
    builder.addItem(d->m_checkBox.data());
    connect(d->m_checkBox.data(), &QAbstractButton::clicked, this, [this] {
        setValue(d->m_checkBox->isChecked());
    });
}

/*!
    \reimp
*/

bool BoolAspect::value() const
{
    return BaseAspect::value().toBool();
}

void BoolAspect::setValue(bool value)
{
    if (BaseAspect::setValueQuietly(value)) {
        if (d->m_checkBox)
            d->m_checkBox->setChecked(value);
        emit changed();
    }
}

void BoolAspect::setLabel(const QString &labelText, LabelPlacement labelPlacement)
{
    BaseAspect::setLabelText(labelText);
    d->m_labelPlacement = labelPlacement;
}

/*!
    \class Utils::SelectionAspect
    \inmodule QtCreator

    \brief A selection aspect represents a specific choice out of
    several.

    The selection aspect is displayed using a QComboBox or
    QRadioButtons in a QButtonGroup.
*/

SelectionAspect::SelectionAspect()
    : d(new Internal::SelectionAspectPrivate)
{}

/*!
    \reimp
*/
SelectionAspect::~SelectionAspect() = default;

/*!
    \reimp
*/
void SelectionAspect::addToLayout(LayoutBuilder &builder)
{
    QTC_CHECK(d->m_buttonGroup == nullptr);
    QTC_CHECK(!d->m_comboBox);
    QTC_ASSERT(d->m_buttons.isEmpty(), d->m_buttons.clear());

    switch (d->m_displayStyle) {
    case DisplayStyle::RadioButtons:
        d->m_buttonGroup = new QButtonGroup();
        d->m_buttonGroup->setExclusive(true);
        for (int i = 0, n = d->m_options.size(); i < n; ++i) {
            const Internal::SelectionAspectPrivate::Option &option = d->m_options.at(i);
            auto button = new QRadioButton(option.displayName);
            button->setChecked(i == value());
            button->setToolTip(option.tooltip);
            builder.addItems({{}, button});
            d->m_buttons.append(button);
            d->m_buttonGroup->addButton(button);
            connect(button, &QAbstractButton::clicked, this, [this, i] {
                setValue(i);
            });
        }
        break;
    case DisplayStyle::ComboBox:
        setupLabel();
        setLabelText(displayName());
        d->m_comboBox = createSubWidget<QComboBox>();
        for (int i = 0, n = d->m_options.size(); i < n; ++i)
            d->m_comboBox->addItem(d->m_options.at(i).displayName);
        connect(d->m_comboBox.data(), QOverload<int>::of(&QComboBox::activated),
                this, &SelectionAspect::setValue);
        d->m_comboBox->setCurrentIndex(value());
        builder.addItems({label(), d->m_comboBox.data()});
        break;
    }
}

void SelectionAspect::setVisibleDynamic(bool visible)
{
    if (QLabel *l = label())
        l->setVisible(visible);
    if (d->m_comboBox)
        d->m_comboBox->setVisible(visible);
    for (QRadioButton * const button : qAsConst(d->m_buttons))
        button->setVisible(visible);
}

void SelectionAspect::setDisplayStyle(SelectionAspect::DisplayStyle style)
{
    d->m_displayStyle = style;
}

int SelectionAspect::value() const
{
    return BaseAspect::value().toInt();
}

void SelectionAspect::setValue(int value)
{
    if (BaseAspect::setValueQuietly(value)) {
        if (d->m_buttonGroup && 0 <= value && value < d->m_buttons.size())
            d->m_buttons.at(value)->setChecked(true);
        else if (d->m_comboBox)
            d->m_comboBox->setCurrentIndex(value);
        emit changed();
    }
}

QString SelectionAspect::stringValue() const
{
    return d->m_options.at(value()).displayName;
}

void SelectionAspect::addOption(const QString &displayName, const QString &toolTip)
{
    d->m_options.append({displayName, toolTip});
}

/*!
    \class Utils::MultiSelectionAspect
    \inmodule QtCreator

    \brief A multi-selection aspect represents one or more choices out of
    several.

    The multi-selection aspect is displayed using a QListWidget with
    checkable items.
*/

MultiSelectionAspect::MultiSelectionAspect()
    : d(new Internal::MultiSelectionAspectPrivate(this))
{
    setValue(QStringList());
    setDefaultValue(QStringList());
}

/*!
    \reimp
*/
MultiSelectionAspect::~MultiSelectionAspect() = default;

/*!
    \reimp
*/
void MultiSelectionAspect::addToLayout(LayoutBuilder &builder)
{
    QTC_CHECK(d->m_listView == nullptr);
    if (d->m_allValues.isEmpty())
        return;

    switch (d->m_displayStyle) {
    case DisplayStyle::ListView:
        setupLabel();
        d->m_listView = createSubWidget<QListWidget>();
        for (const QString &val : qAsConst(d->m_allValues)) {
            auto item = new QListWidgetItem(val, d->m_listView);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(value().contains(item->text()) ? Qt::Checked : Qt::Unchecked);
        }
        connect(d->m_listView, &QListWidget::itemChanged, this,
            [this](QListWidgetItem *item) {
            if (d->setValueSelectedHelper(item->text(), item->checkState() & Qt::Checked))
                emit changed();
        });
        builder.addItems({label(), d->m_listView.data()});
    }
}

bool Internal::MultiSelectionAspectPrivate::setValueSelectedHelper(const QString &val, bool on)
{
    QStringList list = q->value();
    if (on && !list.contains(val)) {
        list.append(val);
        q->setValue(list);
        return true;
    }
    if (!on && list.contains(val)) {
        list.removeOne(val);
        q->setValue(list);
        return true;
    }
    return false;
}

QStringList MultiSelectionAspect::allValues() const
{
    return d->m_allValues;
}

void MultiSelectionAspect::setAllValues(const QStringList &val)
{
    d->m_allValues = val;
}

void MultiSelectionAspect::setVisibleDynamic(bool visible)
{
    if (QLabel *l = label())
        l->setVisible(visible);
    if (d->m_listView)
        d->m_listView->setVisible(visible);
}

void MultiSelectionAspect::setDisplayStyle(MultiSelectionAspect::DisplayStyle style)
{
    d->m_displayStyle = style;
}

QStringList MultiSelectionAspect::value() const
{
    return BaseAspect::value().toStringList();
}

void MultiSelectionAspect::setValue(const QStringList &value)
{
    if (BaseAspect::setValueQuietly(value)) {
        if (d->m_listView) {
            const int n = d->m_listView->count();
            QTC_CHECK(n == d->m_allValues.size());
            for (int i = 0; i != n; ++i) {
                auto item = d->m_listView->item(i);
                item->setCheckState(value.contains(item->text()) ? Qt::Checked : Qt::Unchecked);
            }
        } else {
            emit changed();
        }
    }
}


/*!
    \class Utils::IntegerAspect
    \inmodule QtCreator

    \brief An integer aspect is a integral property of some object, together with
    a description of its behavior for common operations like visualizing or
    persisting.

    The integer aspect is displayed using a \c QSpinBox.

    The visual representation often contains a label in front
    the display of the spin box.
*/

// IntegerAspect

IntegerAspect::IntegerAspect()
    : d(new Internal::IntegerAspectPrivate)
{
    setDefaultValue(qint64(0));
}

/*!
    \reimp
*/
IntegerAspect::~IntegerAspect() = default;

/*!
    \reimp
*/
void IntegerAspect::addToLayout(LayoutBuilder &builder)
{
    setupLabel();

    QTC_CHECK(!d->m_spinBox);
    d->m_spinBox = createSubWidget<QSpinBox>();
    d->m_spinBox->setValue(int(value() / d->m_displayScaleFactor));
    d->m_spinBox->setDisplayIntegerBase(d->m_displayIntegerBase);
    d->m_spinBox->setPrefix(d->m_prefix);
    d->m_spinBox->setSuffix(d->m_suffix);
    if (d->m_maximumValue.isValid() && d->m_maximumValue.isValid())
        d->m_spinBox->setRange(int(d->m_minimumValue.toLongLong() / d->m_displayScaleFactor),
                               int(d->m_maximumValue.toLongLong() / d->m_displayScaleFactor));

    builder.addItems({label(), d->m_spinBox.data()});
    connect(d->m_spinBox.data(), QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int value) {
        setValue(value * d->m_displayScaleFactor);
    });
}

qint64 IntegerAspect::value() const
{
    return BaseAspect::value().toLongLong();
}

void IntegerAspect::setValue(qint64 value)
{
    if (setValueQuietly(value)) {
        if (d->m_spinBox)
            d->m_spinBox->setValue(int(value / d->m_displayScaleFactor));
        emit changed();
    }
}

void IntegerAspect::setRange(qint64 min, qint64 max)
{
    d->m_minimumValue = min;
    d->m_maximumValue = max;
}

void IntegerAspect::setLabel(const QString &label)
{
    setLabelText(label);
}

void IntegerAspect::setPrefix(const QString &prefix)
{
    d->m_prefix = prefix;
}

void IntegerAspect::setSuffix(const QString &suffix)
{
    d->m_suffix = suffix;
}

void IntegerAspect::setDisplayIntegerBase(int base)
{
    d->m_displayIntegerBase = base;
}

void IntegerAspect::setDisplayScaleFactor(qint64 factor)
{
    d->m_displayScaleFactor = factor;
}

void IntegerAspect::setDefaultValue(qint64 defaultValue)
{
    BaseAspect::setDefaultValue(defaultValue);
}

/*!
    \class Utils::BaseTristateAspect
    \inmodule QtCreator

    \brief A tristate aspect is a property of some object that can have
    three values: enabled, disabled, and unspecified.

    Its visual representation is a QComboBox with three items.
*/

TriStateAspect::TriStateAspect(const QString onString, const QString &offString,
                               const QString &defaultString)
{
    setDisplayStyle(DisplayStyle::ComboBox);
    setValue(TriState::Default);
    setDefaultValue(TriState::Default);
    addOption(onString);
    addOption(offString);
    addOption(defaultString);
}

TriState TriStateAspect::value() const
{
    return TriState::fromVariant(BaseAspect::value());
}

void TriStateAspect::setValue(TriState value)
{
    BaseAspect::setValue(value.toVariant());
}

void TriStateAspect::setDefaultValue(TriState value)
{
    BaseAspect::setDefaultValue(value.toVariant());
}

const TriState TriState::Enabled{TriState::EnabledValue};
const TriState TriState::Disabled{TriState::DisabledValue};
const TriState TriState::Default{TriState::DefaultValue};

TriState TriState::fromVariant(const QVariant &variant)
{
    int v = variant.toInt();
    QTC_ASSERT(v == EnabledValue || v == DisabledValue || v == DefaultValue, v = DefaultValue);
    return TriState(Value(v));
}


/*!
    \class Utils::StringListAspect
    \inmodule QtCreator

    \brief A string list aspect represents a property of some object
    that is a list of strings.
*/

StringListAspect::StringListAspect()
    : d(new Internal::StringListAspectPrivate)
{
    setDefaultValue(QStringList());
}

/*!
    \reimp
*/
StringListAspect::~StringListAspect() = default;

/*!
    \reimp
*/
void StringListAspect::addToLayout(LayoutBuilder &builder)
{
    Q_UNUSED(builder)
    // TODO - when needed.
}

QStringList StringListAspect::value() const
{
    return BaseAspect::value().toStringList();
}

void StringListAspect::setValue(const QStringList &value)
{
    BaseAspect::setValue(value);
}

/*!
    \class Utils::TextDisplay

    \brief A text display is a phony aspect with the sole purpose of providing
    some text display using an Utils::InfoLabel in places where otherwise
    more expensive Utils::StringAspect items would be used.

    A text display does not have a real value.
*/

/*!
    Constructs a text display showing the \a message with an icon representing
    type \a type.
 */
TextDisplay::TextDisplay(const QString &message, InfoLabel::InfoType type)
    : d(new Internal::TextDisplayPrivate)
{
    d->m_message = message;
    d->m_type = type;
}

/*!
    \reimp
*/
TextDisplay::~TextDisplay() = default;

/*!
    \reimp
*/
void TextDisplay::addToLayout(LayoutBuilder &builder)
{
    if (!d->m_label) {
        d->m_label = createSubWidget<InfoLabel>(d->m_message, d->m_type);
        d->m_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
        d->m_label->setElideMode(Qt::ElideNone);
        d->m_label->setWordWrap(true);
    }
    builder.addItem(d->m_label.data());
    d->m_label->setVisible(isVisible());
}

/*!
    Sets \a t as the information label type for the visual representation
    of this aspect.
 */
void TextDisplay::setIconType(InfoLabel::InfoType t)
{
    d->m_type = t;
    if (d->m_label)
        d->m_label->setType(t);
}

/*!
    \class Utils::AspectContainer
    \inmodule QtCreator

    \brief The AspectContainer class wraps one or more aspects while providing
    the interface of a single aspect.
*/

AspectContainer::AspectContainer()
    : d(new Internal::AspectContainerPrivate)
{}

/*!
    \reimp
*/
AspectContainer::~AspectContainer() = default;

/*!
    \internal
*/
void AspectContainer::addAspectHelper(BaseAspect *aspect)
{
    d->m_items.append(aspect);
    connect(aspect, &BaseAspect::changed, this, &BaseAspect::changed);
}

/*!
    Adds all visible sub-aspects to \a builder.
*/
void AspectContainer::addToLayout(LayoutBuilder &builder)
{
    for (BaseAspect *aspect : qAsConst(d->m_items)) {
        if (aspect->isVisible())
            aspect->addToLayout(builder);
    }
}

/*!
    \reimp
*/
void AspectContainer::fromMap(const QVariantMap &map)
{
    for (BaseAspect *aspect : qAsConst(d->m_items))
        aspect->fromMap(map);
}

/*!
    \reimp
*/
void AspectContainer::toMap(QVariantMap &map) const
{
    for (BaseAspect *aspect : qAsConst(d->m_items))
        aspect->toMap(map);
}

} // namespace Utils
