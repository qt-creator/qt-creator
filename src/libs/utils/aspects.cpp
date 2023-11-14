// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "aspects.h"

#include "algorithm.h"
#include "checkablemessagebox.h"
#include "environment.h"
#include "fancylineedit.h"
#include "iconbutton.h"
#include "layoutbuilder.h"
#include "passworddialog.h"
#include "pathchooser.h"
#include "pathlisteditor.h"
#include "qtcassert.h"
#include "qtcolorbutton.h"
#include "qtcsettings.h"
#include "utilsicons.h"
#include "utilstr.h"
#include "variablechooser.h"

#include <QAction>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QCompleter>
#include <QDebug>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPaintEvent>
#include <QPainter>
#include <QPointer>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QStandardItemModel>
#include <QTextEdit>
#include <QUndoStack>

using namespace Layouting;

namespace Utils {

static QtcSettings *theSettings = nullptr;

void BaseAspect::setQtcSettings(QtcSettings *settings)
{
    theSettings = settings;
}

QtcSettings *BaseAspect::qtcSettings()
{
    return theSettings;
}

BaseAspect::Changes::Changes()
{
    memset(this, 0, sizeof(*this));
}

class Internal::BaseAspectPrivate
{
public:
    explicit BaseAspectPrivate(AspectContainer *container) : m_container(container) {}

    Id m_id;
    std::function<QVariant(const QVariant &)> m_toSettings;
    std::function<QVariant(const QVariant &)> m_fromSettings;

    QString m_displayName;
    Key m_settingsKey; // Name of data in settings.
    QString m_tooltip;
    QString m_labelText;
    QPixmap m_labelPixmap;
    QIcon m_icon;
    QPointer<QAction> m_action; // Owned by us.
    AspectContainer *m_container = nullptr; // Not owned by us.

    bool m_visible = true;
    bool m_enabled = true;
    bool m_readOnly = false;
    bool m_autoApply = true;
    bool m_hasEnabler = false;
    int m_spanX = 1;
    int m_spanY = 1;
    BaseAspect::ConfigWidgetCreator m_configWidgetCreator;
    QList<QPointer<QWidget>> m_subWidgets;

    BaseAspect::DataCreator m_dataCreator;
    BaseAspect::DataCloner m_dataCloner;
    QList<BaseAspect::DataExtractor> m_dataExtractors;

    QUndoStack *m_undoStack = nullptr;
};

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
    Constructs a base aspect.

    If \a container is non-null, the aspect is made known to the container.
*/
BaseAspect::BaseAspect(AspectContainer *container)
    : d(new Internal::BaseAspectPrivate(container))
{
    if (container)
        container->registerAspect(this);
    addDataExtractor(this, &BaseAspect::variantValue, &Data::value);
}

/*!
    Destructs a BaseAspect.
*/
BaseAspect::~BaseAspect()
{
    delete d->m_action;
}

Id BaseAspect::id() const
{
    return d->m_id;
}

void BaseAspect::setId(Id id)
{
    d->m_id = id;
}

QVariant BaseAspect::volatileVariantValue() const
{
    return {};
}

QVariant BaseAspect::variantValue() const
{
    return {};
}

/*!
    Sets \a value.

    Prefer the typed setValue() of derived classes.
*/
void BaseAspect::setVariantValue(const QVariant &value, Announcement howToAnnounce)
{
    Q_UNUSED(value)
    Q_UNUSED(howToAnnounce)
    QTC_CHECK(false);
}

void BaseAspect::setDefaultVariantValue(const QVariant &value)
{
    Q_UNUSED(value)
    QTC_CHECK(false);
}

bool BaseAspect::isDefaultValue() const
{
    return defaultVariantValue() == variantValue();
}

QVariant BaseAspect::defaultVariantValue() const
{
    return {};
}

/*!
    \fn TypedAspect::setDefaultValue(const ValueType &value)

    Sets a default \a value and the current value for this aspect.

    \note The current value will be set silently to the same value.
    It is reasonable to only set default values in the setup phase
    of the aspect.

    Default values will not be stored in settings.
*/

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
    for (QWidget *w : std::as_const(d->m_subWidgets)) {
        QTC_ASSERT(w, continue);
        // This may happen during layout building. Explicit setting visibility here
        // may create a show a toplevel widget for a moment until it is parented
        // to some non-shown widget.
        if (!visible || w->parentWidget())
            w->setVisible(visible);
    }
}

QLabel *BaseAspect::createLabel()
{
    if (d->m_labelText.isEmpty() && d->m_labelPixmap.isNull())
        return nullptr;

    auto label = new QLabel(d->m_labelText);
    label->setTextInteractionFlags(label->textInteractionFlags() | Qt::TextSelectableByMouse);
    connect(label, &QLabel::linkActivated, this, [this](const QString &link) {
        emit labelLinkActivated(link);
    });
    if (!d->m_labelPixmap.isNull())
        label->setPixmap(d->m_labelPixmap);
    registerSubWidget(label);

    connect(this, &BaseAspect::labelTextChanged, label, [label, this] {
        label->setText(d->m_labelText);
    });
    connect(this, &BaseAspect::labelPixmapChanged, label, [label, this] {
        label->setPixmap(d->m_labelPixmap);
    });

    return label;
}

void BaseAspect::addLabeledItem(LayoutItem &parent, QWidget *widget)
{
    if (QLabel *l = createLabel()) {
        l->setBuddy(widget);
        parent.addItem(l);
        parent.addItem(Span(std::max(d->m_spanX - 1, 1), LayoutItem(widget)));
    } else {
        parent.addItem(LayoutItem(widget));
    }
}

/*!
    Sets \a labelText as text for the separate label in the visual
    representation of this aspect.
*/
void BaseAspect::setLabelText(const QString &labelText)
{
    d->m_labelText = labelText;
    emit labelTextChanged();
}

/*!
    Sets \a labelPixmap as pixmap for the separate label in the visual
    representation of this aspect.
*/
void BaseAspect::setLabelPixmap(const QPixmap &labelPixmap)
{
    d->m_labelPixmap = labelPixmap;
    emit labelPixmapChanged();
}

void BaseAspect::setIcon(const QIcon &icon)
{
    d->m_icon = icon;
    if (d->m_action)
        d->m_action->setIcon(icon);
}

/*!
    Returns the current text for the separate label in the visual
    representation of this aspect.
*/
QString BaseAspect::labelText() const
{
    return d->m_labelText;
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
    for (QWidget *w : std::as_const(d->m_subWidgets)) {
        QTC_ASSERT(w, continue);
        w->setToolTip(tooltip);
    }
}

void BaseAspect::setUndoStack(QUndoStack *undoStack)
{
    d->m_undoStack = undoStack;
}

QUndoStack *BaseAspect::undoStack() const
{
    return d->m_undoStack;
}

bool BaseAspect::isEnabled() const
{
    return d->m_enabled;
}

void BaseAspect::setEnabled(bool enabled)
{
    for (QWidget *w : std::as_const(d->m_subWidgets)) {
        QTC_ASSERT(w, continue);
        w->setEnabled(enabled);
    }

    if (enabled == d->m_enabled)
        return;

    d->m_enabled = enabled;
    emit enabledChanged();
}

/*!
    Makes the enabled state of this aspect depend on the checked state of \a checker.
*/
void BaseAspect::setEnabler(BoolAspect *checker)
{
    QTC_ASSERT(checker, return);

    d->m_hasEnabler = true;

    auto update = [this, checker] {
        BaseAspect::setEnabled(checker->isEnabled() && checker->volatileValue());
    };

    connect(checker, &BoolAspect::volatileValueChanged, this, update);
    connect(checker, &BoolAspect::changed, this, update);
    connect(checker, &BaseAspect::enabledChanged, this, update);

    update();
}

bool BaseAspect::isReadOnly() const
{
    return d->m_readOnly;
}

void BaseAspect::setReadOnly(bool readOnly)
{
    d->m_readOnly = readOnly;
    for (QWidget *w : std::as_const(d->m_subWidgets)) {
        QTC_ASSERT(w, continue);
        if (auto lineEdit = qobject_cast<QLineEdit *>(w))
            lineEdit->setReadOnly(readOnly);
        else if (auto textEdit = qobject_cast<QTextEdit *>(w))
            textEdit->setReadOnly(readOnly);
        else if (auto pathChooser = qobject_cast<PathChooser *>(w))
            pathChooser->setReadOnly(readOnly);
    }
}

void BaseAspect::setSpan(int x, int y)
{
    d->m_spanX = x;
    d->m_spanY = y;
}

bool BaseAspect::isAutoApply() const
{
    return d->m_autoApply;
}

/*!
    Sets auto-apply mode. When auto-apply mode is \a on, user interaction to this
    aspect's widget will not modify the \c value of the aspect until \c apply()
    is called programmatically.

    \sa setSettingsKey()
*/

void BaseAspect::setAutoApply(bool on)
{
    d->m_autoApply = on;
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
Key BaseAspect::settingsKey() const
{
    return d->m_settingsKey;
}

/*!
    Sets the \a key to be used when accessing the settings.

    \sa settingsKey()
*/
void BaseAspect::setSettingsKey(const Key &key)
{
    d->m_settingsKey = key;
}

/*!
    Sets the \a key and \a group to be used when accessing the settings.

    \sa settingsKey()
*/
void BaseAspect::setSettingsKey(const Key &group, const Key &key)
{
    d->m_settingsKey = group + "/" + key;
}

/*!
    Immediately writes the value of this aspect into its specified
    settings, taking a potential container's settings group specification
    into account.

    \note This is expensive, so it should only be used with good reason.
*/
void BaseAspect::writeToSettingsImmediatly() const
{
    QStringList groups;
    if (d->m_container)
        groups = d->m_container->settingsGroups();
    const SettingsGroupNester nester(groups);
    writeSettings();
}

/*!
    Returns the string that should be used when this action appears in menus
    or other places that are typically used with Book style capitalization.

    If no display name is set, the label text will be used as fallback.
*/

QString BaseAspect::displayName() const
{
    return d->m_displayName.isEmpty() ? d->m_labelText : d->m_displayName;
}

/*!
    \internal
*/
QWidget *BaseAspect::createConfigWidget() const
{
    return d->m_configWidgetCreator ? d->m_configWidgetCreator() : nullptr;
}

QAction *BaseAspect::action()
{
    if (!d->m_action) {
        d->m_action = new QAction(labelText());
        d->m_action->setIcon(d->m_icon);
    }
    return d->m_action;
}

AspectContainer *BaseAspect::container() const
{
    return d->m_container;
}

/*!
    Adds the visual representation of this aspect to the layout with the
    specified \a parent using a layout builder.
*/
void BaseAspect::addToLayout(LayoutItem &)
{
}

void createItem(Layouting::LayoutItem *item, const BaseAspect &aspect)
{
    const_cast<BaseAspect &>(aspect).addToLayout(*item);
}

void createItem(Layouting::LayoutItem *item, const BaseAspect *aspect)
{
    const_cast<BaseAspect *>(aspect)->addToLayout(*item);
}


/*!
    Updates this aspect's value from user-initiated changes in the widget.

    Emits changed() if the value changed.
*/
void BaseAspect::apply()
{
    // We assume m_buffer to reflect current gui state as invariant after signalling settled down.
    // It's an aspect (-subclass) implementation problem if this doesn't hold. Fix it up and bark.
    QTC_CHECK(!guiToBuffer());

    if (!bufferToInternal()) // Nothing to do.
        return;

    Changes changes;
    changes.internalFromBuffer = true;
    announceChanges(changes);
}

/*!
    Discard user changes in the widget and restore widget contents from
    aspect's value.

    This has only an effect if \c isAutoApply is false.
*/
void BaseAspect::cancel()
{
    internalToBuffer();
    bufferToGui();
}

void BaseAspect::finish()
{
    // No qDeleteAll() possible as long as the connect in registerSubWidget() exist.
    while (d->m_subWidgets.size())
        delete d->m_subWidgets.takeLast();
}

bool BaseAspect::hasAction() const
{
    return d->m_action != nullptr;
}

void BaseAspect::announceChanges(Changes changes, Announcement howToAnnounce)
{
    if (howToAnnounce == BeQuiet)
        return;

    if (changes.bufferFromInternal || changes.bufferFromOutside || changes.bufferFromGui)
        emit volatileValueChanged();

    if (changes.internalFromOutside || changes.internalFromBuffer) {
        emit changed();
        if (hasAction())
            emit action()->triggered(variantValue().toBool());
    }
}

bool BaseAspect::isDirty()
{
    return false;
}

void BaseAspect::registerSubWidget(QWidget *widget)
{
    d->m_subWidgets.append(widget);

    // FIXME: This interferes with qDeleteAll() in finish() and destructor,
    // it would not be needed when all users actually deleted their subwidgets,
    // e.g. the SettingsPage::finish() base implementation, but this still
    // leaves the cases where no such base functionality is available, e.g.
    // in the run/build config aspects.
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

void BaseAspect::forEachSubWidget(const std::function<void(QWidget *)> &func)
{
    for (const QPointer<QWidget> &w : d->m_subWidgets)
        func(w);
}

void BaseAspect::saveToMap(Store &data, const QVariant &value,
                           const QVariant &defaultValue, const Key &key)
{
    if (key.isEmpty())
        return;
    if (value == defaultValue)
        data.remove(key);
    else
        data.insert(key, value);
}

/*!
    Retrieves the internal value of this BaseAspect from the Store \a map.
*/
void BaseAspect::fromMap(const Store &map)
{
    if (settingsKey().isEmpty())
        return;
    const QVariant val = map.value(settingsKey(), toSettingsValue(defaultVariantValue()));
    setVariantValue(fromSettingsValue(val), BeQuiet);
}

/*!
    Stores the internal value of this BaseAspect into the Store \a map.
*/
void BaseAspect::toMap(Store &map) const
{
    if (settingsKey().isEmpty())
        return;
    saveToMap(map, toSettingsValue(variantValue()), toSettingsValue(defaultVariantValue()), settingsKey());
}

void BaseAspect::volatileToMap(Store &map) const
{
    if (settingsKey().isEmpty())
        return;
    saveToMap(map,
              toSettingsValue(volatileVariantValue()),
              toSettingsValue(defaultVariantValue()),
              settingsKey());
}

void BaseAspect::readSettings()
{
    if (settingsKey().isEmpty())
        return;
    QTC_ASSERT(theSettings, return);
    // The enabler needs to be set up after reading the settings, otherwise
    // changes from reading the settings will not update the enabled state
    // because the updates are "quiet".
    QTC_CHECK(!d->m_hasEnabler);
    const QVariant val = theSettings->value(settingsKey());
    setVariantValue(val.isValid() ? fromSettingsValue(val) : defaultVariantValue(), BeQuiet);
}

void BaseAspect::writeSettings() const
{
    if (settingsKey().isEmpty())
        return;
    QTC_ASSERT(theSettings, return);
    theSettings->setValueWithDefault(settingsKey(),
                                     toSettingsValue(variantValue()),
                                     toSettingsValue(defaultVariantValue()));
}

void BaseAspect::setFromSettingsTransformation(const SavedValueTransformation &transform)
{
    d->m_fromSettings = transform;
}

void BaseAspect::setToSettingsTransformation(const SavedValueTransformation &transform)
{
    d->m_toSettings = transform;
}

QVariant BaseAspect::toSettingsValue(const QVariant &val) const
{
    return d->m_toSettings ? d->m_toSettings(val) : val;
}

QVariant BaseAspect::fromSettingsValue(const QVariant &val) const
{
    return d->m_fromSettings ? d->m_fromSettings(val) : val;
}

namespace Internal {

class BoolAspectPrivate
{
public:
    BoolAspect::LabelPlacement m_labelPlacement = BoolAspect::LabelPlacement::AtCheckBox;
    UndoableValue<bool> m_undoable;
};

class ColorAspectPrivate
{
public:
    QPointer<QtColorButton> m_colorButton; // Owned by configuration widget
};

class SelectionAspectPrivate
{
public:
    ~SelectionAspectPrivate() { delete m_buttonGroup; }

    SelectionAspect::DisplayStyle m_displayStyle
            = SelectionAspect::DisplayStyle::RadioButtons;
    QList<SelectionAspect::Option> m_options;

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

class CheckableAspectImplementation
{
public:
    void fromMap(const Store &map)
    {
        if (m_checked)
            m_checked->fromMap(map);
    }

    void toMap(Store &map)
    {
        if (m_checked)
            m_checked->toMap(map);
    }

    void volatileToMap(Store &map)
    {
        if (m_checked)
            m_checked->volatileToMap(map);
    }

    template<class Widget>
    void updateWidgetFromCheckStatus(BaseAspect *aspect, Widget *w)
    {
        const bool enabled = !m_checked || m_checked->value();
        if (m_uncheckedSemantics == UncheckedSemantics::Disabled)
            w->setEnabled(enabled && aspect->isEnabled());
        else
            w->setReadOnly(!enabled || aspect->isReadOnly());
    }

    void setUncheckedSemantics(UncheckedSemantics semantics)
    {
        m_uncheckedSemantics = semantics;
    }

    bool isChecked() const
    {
        QTC_ASSERT(m_checked, return false);
        return m_checked->value();
    }

    void setChecked(bool checked)
    {
        QTC_ASSERT(m_checked, return);
        m_checked->setValue(checked);
    }

    void makeCheckable(CheckBoxPlacement checkBoxPlacement, const QString &checkerLabel,
                       const Key &checkerKey, BaseAspect *aspect)
    {
        QTC_ASSERT(!m_checked, return);
        m_checkBoxPlacement = checkBoxPlacement;
        m_checked.reset(new BoolAspect);
        m_checked->setLabel(checkerLabel, checkBoxPlacement == CheckBoxPlacement::Top
                                              ? BoolAspect::LabelPlacement::InExtraLabel
                                              : BoolAspect::LabelPlacement::AtCheckBox);
        m_checked->setSettingsKey(checkerKey);

        QObject::connect(m_checked.get(), &BoolAspect::changed, aspect, [aspect] {
            // FIXME: Check.
            aspect->internalToBuffer();
            aspect->bufferToGui();
            emit aspect->changed();
            aspect->checkedChanged();
        });

        QObject::connect(m_checked.get(), &BoolAspect::volatileValueChanged, aspect, [aspect] {
            // FIXME: Check.
            aspect->internalToBuffer();
            aspect->bufferToGui();
            aspect->checkedChanged();
        });

        aspect->internalToBuffer();
        aspect->bufferToGui();
    }

    void addToLayoutFirst(LayoutItem &parent)
    {
        if (m_checked && m_checkBoxPlacement == CheckBoxPlacement::Top) {
            m_checked->addToLayout(parent);
            parent.addItem(br);
        }
    }

    void addToLayoutLast(LayoutItem &parent)
    {
        if (m_checked && m_checkBoxPlacement == CheckBoxPlacement::Right)
            m_checked->addToLayout(parent);
    }

    CheckBoxPlacement m_checkBoxPlacement = CheckBoxPlacement::Right;
    UncheckedSemantics m_uncheckedSemantics = UncheckedSemantics::Disabled;
    std::unique_ptr<BoolAspect> m_checked;
};

class StringAspectPrivate
{
public:
    StringAspect::DisplayStyle m_displayStyle = StringAspect::LabelDisplay;
    std::function<QString(const QString &)> m_displayFilter;

    Qt::TextElideMode m_elideMode = Qt::ElideNone;
    QString m_placeHolderText;
    Key m_historyCompleterKey;
    MacroExpanderProvider m_expanderProvider;
    StringAspect::ValueAcceptor m_valueAcceptor;
    std::optional<FancyLineEdit::ValidationFunction> m_validator;

    CheckableAspectImplementation m_checkerImpl;

    bool m_undoRedoEnabled = true;
    bool m_acceptRichText = false;
    bool m_showToolTipOnLabel = false;
    bool m_useResetButton = false;
    bool m_autoApplyOnEditingFinished = false;
    bool m_validatePlaceHolder = false;

    UndoableValue<QString> undoable;
};

class IntegerAspectPrivate
{
public:
    std::optional<qint64> m_minimumValue;
    std::optional<qint64> m_maximumValue;
    int m_displayIntegerBase = 10;
    qint64 m_displayScaleFactor = 1;
    QString m_prefix;
    QString m_suffix;
    QString m_specialValueText;
    int m_singleStep = 1;
    QPointer<QSpinBox> m_spinBox; // Owned by configuration widget
};

class DoubleAspectPrivate
{
public:
    std::optional<double> m_minimumValue;
    std::optional<double> m_maximumValue;
    QString m_prefix;
    QString m_suffix;
    QString m_specialValueText;
    double m_singleStep = 1;
    QPointer<QDoubleSpinBox> m_spinBox; // Owned by configuration widget
};

class StringListAspectPrivate
{
public:
};

class FilePathListAspectPrivate
{
public:
    UndoableValue<QStringList> undoable;
    QString placeHolderText;
};

class TextDisplayPrivate
{
public:
    QString m_message;
    InfoLabel::InfoType m_type;
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

      \value PasswordLineEditDisplay
             Based on QLineEdit, used for password strings

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
    Constructs the string aspect \a container.
 */

StringAspect::StringAspect(AspectContainer *container)
    : TypedAspect(container), d(new Internal::StringAspectPrivate)
{
    setSpan(2, 1); // Default: Label + something
}

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
    \reimp
*/
void StringAspect::fromMap(const Store &map)
{
    if (!settingsKey().isEmpty())
        setValue(map.value(settingsKey(), defaultValue()).toString(), BeQuiet);
    d->m_checkerImpl.fromMap(map);
}

/*!
    \reimp
*/
void StringAspect::toMap(Store &map) const
{
    saveToMap(map, value(), defaultValue(), settingsKey());
    d->m_checkerImpl.toMap(map);
}

void StringAspect::volatileToMap(Store &map) const
{
    saveToMap(map, volatileValue(), defaultValue(), settingsKey());
    d->m_checkerImpl.volatileToMap(map);
}

/*!
    \internal
*/
void StringAspect::setShowToolTipOnLabel(bool show)
{
    d->m_showToolTipOnLabel = show;
    bufferToGui();
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
    if (d->m_placeHolderText == placeHolderText)
        return;

    d->m_placeHolderText = placeHolderText;
    emit placeholderTextChanged(placeHolderText);
}

/*!
    Sets \a elideMode as label elide mode.
*/
void StringAspect::setElideMode(Qt::TextElideMode elideMode)
{
    if (d->m_elideMode == elideMode)
        return;
    d->m_elideMode = elideMode;
    emit elideModeChanged(elideMode);
}

/*!
    Sets \a historyCompleterKey as key for the history completer settings for
    line edits and path chooser displays.

    \sa Utils::PathChooser::setExpectedKind()
*/
void StringAspect::setHistoryCompleter(const Key &historyCompleterKey)
{
    d->m_historyCompleterKey = historyCompleterKey;
    emit historyCompleterKeyChanged(historyCompleterKey);
}

void StringAspect::setAcceptRichText(bool acceptRichText)
{
    d->m_acceptRichText = acceptRichText;
    emit acceptRichTextChanged(acceptRichText);
}

void StringAspect::setMacroExpanderProvider(const MacroExpanderProvider &expanderProvider)
{
    d->m_expanderProvider = expanderProvider;
}

void StringAspect::setUseGlobalMacroExpander()
{
    d->m_expanderProvider = &globalMacroExpander;
}

void StringAspect::setUseResetButton()
{
    d->m_useResetButton = true;
}

void StringAspect::setValidationFunction(const FancyLineEdit::ValidationFunction &validator)
{
    d->m_validator = validator;
    emit validationFunctionChanged(validator);
}

void StringAspect::setAutoApplyOnEditingFinished(bool applyOnEditingFinished)
{
    d->m_autoApplyOnEditingFinished = applyOnEditingFinished;
}

void StringAspect::addToLayout(LayoutItem &parent)
{
    d->m_checkerImpl.addToLayoutFirst(parent);

    const auto useMacroExpander = [this](QWidget *w) {
        if (!d->m_expanderProvider)
            return;
        const auto chooser = new VariableChooser(w);
        chooser->addSupportedWidget(w);
        chooser->addMacroExpanderProvider(d->m_expanderProvider);
    };

    const QString displayedString = d->m_displayFilter ? d->m_displayFilter(value()) : value();

    switch (d->m_displayStyle) {
    case PasswordLineEditDisplay:
    case LineEditDisplay: {
        auto lineEditDisplay = createSubWidget<FancyLineEdit>();
        lineEditDisplay->setPlaceholderText(d->m_placeHolderText);
        if (!d->m_historyCompleterKey.isEmpty())
            lineEditDisplay->setHistoryCompleter(d->m_historyCompleterKey);

        connect(this,
                &StringAspect::historyCompleterKeyChanged,
                lineEditDisplay,
                [lineEditDisplay](const Key &historyCompleterKey) {
                    lineEditDisplay->setHistoryCompleter(historyCompleterKey);
                });
        connect(this,
                &StringAspect::placeholderTextChanged,
                lineEditDisplay,
                &FancyLineEdit::setPlaceholderText);

        if (d->m_validator)
            lineEditDisplay->setValidationFunction(*d->m_validator);
        lineEditDisplay->setTextKeepingActiveCursor(displayedString);
        lineEditDisplay->setReadOnly(isReadOnly());
        lineEditDisplay->setValidatePlaceHolder(d->m_validatePlaceHolder);

        d->m_checkerImpl.updateWidgetFromCheckStatus(this, lineEditDisplay);

        if (d->m_checkerImpl.m_checked.get()) {
            connect(d->m_checkerImpl.m_checked.get(),
                    &BoolAspect::volatileValueChanged,
                    lineEditDisplay,
                    [this, lineEditDisplay]() {
                        d->m_checkerImpl.updateWidgetFromCheckStatus(this, lineEditDisplay);
                    });
        }

        addLabeledItem(parent, lineEditDisplay);
        useMacroExpander(lineEditDisplay);
        if (d->m_useResetButton) {
            auto resetButton = createSubWidget<QPushButton>(Tr::tr("Reset"));
            resetButton->setEnabled(lineEditDisplay->text() != defaultValue());
            connect(resetButton, &QPushButton::clicked, lineEditDisplay, [this, lineEditDisplay] {
                lineEditDisplay->setText(defaultValue());
            });
            connect(lineEditDisplay,
                    &QLineEdit::textChanged,
                    resetButton,
                    [this, lineEditDisplay, resetButton] {
                        resetButton->setEnabled(lineEditDisplay->text() != defaultValue());
                    });
            parent.addItem(resetButton);
        }
        connect(lineEditDisplay, &FancyLineEdit::validChanged, this, &StringAspect::validChanged);
        bufferToGui();
        if (isAutoApply() && d->m_autoApplyOnEditingFinished) {
            connect(lineEditDisplay,
                    &FancyLineEdit::editingFinished,
                    this,
                    [this, lineEditDisplay]() {
                        d->undoable.set(undoStack(), lineEditDisplay->text());
                        handleGuiChanged();
                    });
        } else {
            connect(lineEditDisplay, &QLineEdit::textChanged, this, [this, lineEditDisplay]() {
                d->undoable.set(undoStack(), lineEditDisplay->text());
                handleGuiChanged();
            });
        }
        if (d->m_displayStyle == PasswordLineEditDisplay) {
            auto showPasswordButton = createSubWidget<ShowPasswordButton>();
            lineEditDisplay->setEchoMode(QLineEdit::PasswordEchoOnEdit);
            parent.addItem(showPasswordButton);
            connect(showPasswordButton,
                    &ShowPasswordButton::toggled,
                    lineEditDisplay,
                    [showPasswordButton, lineEditDisplay] {
                        lineEditDisplay->setEchoMode(showPasswordButton->isChecked()
                                                         ? QLineEdit::Normal
                                                         : QLineEdit::PasswordEchoOnEdit);
                    });
        }

        connect(&d->undoable.m_signal,
                &UndoSignaller::changed,
                lineEditDisplay,
                [this, lineEditDisplay] {
                    if (lineEditDisplay->text() != d->undoable.get())
                        lineEditDisplay->setTextKeepingActiveCursor(d->undoable.get());

                    lineEditDisplay->validate();
                });

        break;
    }
    case TextEditDisplay: {
        auto textEditDisplay = createSubWidget<QTextEdit>();
        textEditDisplay->setPlaceholderText(d->m_placeHolderText);
        textEditDisplay->setUndoRedoEnabled(false);
        textEditDisplay->setAcceptRichText(d->m_acceptRichText);
        textEditDisplay->setTextInteractionFlags(Qt::TextEditorInteraction);
        textEditDisplay->setText(displayedString);
        textEditDisplay->setReadOnly(isReadOnly());
        d->m_checkerImpl.updateWidgetFromCheckStatus(this, textEditDisplay);

        if (d->m_checkerImpl.m_checked) {
            connect(d->m_checkerImpl.m_checked.get(),
                    &BoolAspect::volatileValueChanged,
                    textEditDisplay,
                    [this, textEditDisplay]() {
                        d->m_checkerImpl.updateWidgetFromCheckStatus(this, textEditDisplay);
                    });
        }

        addLabeledItem(parent, textEditDisplay);
        useMacroExpander(textEditDisplay);
        bufferToGui();
        connect(this,
                &StringAspect::acceptRichTextChanged,
                textEditDisplay,
                &QTextEdit::setAcceptRichText);
        connect(this,
                &StringAspect::placeholderTextChanged,
                textEditDisplay,
                &QTextEdit::setPlaceholderText);

        connect(textEditDisplay, &QTextEdit::textChanged, this, [this, textEditDisplay]() {
            if (textEditDisplay->toPlainText() != d->undoable.get()) {
                d->undoable.set(undoStack(), textEditDisplay->toPlainText());
                handleGuiChanged();
            }
        });

        connect(&d->undoable.m_signal,
                &UndoSignaller::changed,
                textEditDisplay,
                [this, textEditDisplay] {
                    if (textEditDisplay->toPlainText() != d->undoable.get())
                        textEditDisplay->setText(d->undoable.get());
                });
        break;
    }
    case LabelDisplay: {
        auto labelDisplay = createSubWidget<ElidingLabel>();
        labelDisplay->setElideMode(d->m_elideMode);
        labelDisplay->setTextInteractionFlags(Qt::TextSelectableByMouse);
        labelDisplay->setText(displayedString);
        labelDisplay->setToolTip(d->m_showToolTipOnLabel ? displayedString : toolTip());
        connect(this, &StringAspect::elideModeChanged, labelDisplay, &ElidingLabel::setElideMode);
        addLabeledItem(parent, labelDisplay);

        connect(&d->undoable.m_signal, &UndoSignaller::changed, labelDisplay, [this, labelDisplay] {
            labelDisplay->setText(d->undoable.get());
            labelDisplay->setToolTip(d->m_showToolTipOnLabel ? d->undoable.get() : toolTip());
        });

        break;
    }
    }

    d->m_checkerImpl.addToLayoutLast(parent);
}

QString StringAspect::expandedValue() const
{
    // FIXME: Use macroexpander here later.
    return m_internal;
}

bool StringAspect::guiToBuffer()
{
    return updateStorage(m_buffer, d->undoable.get());
}

bool StringAspect::bufferToInternal()
{
    if (d->m_valueAcceptor) {
        if (const std::optional<QString> tmp = d->m_valueAcceptor(m_internal, m_buffer))
           return updateStorage(m_internal, *tmp);
    }
    return updateStorage(m_internal, m_buffer);
}

bool StringAspect::internalToBuffer()
{
    const QString val = d->m_displayFilter ? d->m_displayFilter(m_internal) : m_internal;
    return updateStorage(m_buffer, val);
}

void StringAspect::bufferToGui()
{
    d->undoable.setWithoutUndo(m_buffer);
}

/*!
    Adds a check box with a \a checkerLabel according to \a checkBoxPlacement
    to the line edit.

    The state of the check box is made persistent when using a non-emtpy
    \a checkerKey.
*/
void StringAspect::makeCheckable(CheckBoxPlacement checkBoxPlacement,
                                 const QString &checkerLabel, const Key &checkerKey)
{
    d->m_checkerImpl.makeCheckable(checkBoxPlacement, checkerLabel, checkerKey, this);
}

bool StringAspect::isChecked() const
{
    return d->m_checkerImpl.isChecked();
}

void StringAspect::setChecked(bool checked)
{
    return d->m_checkerImpl.setChecked(checked);
}


/*!
    \class Utils::FilePathAspect
    \inmodule QtCreator

    \brief A file path aspect is shallow wrapper around a Utils::StringAspect that
    represents a file in the file system.

    It is displayed by default using Utils::PathChooser.

    The visual representation often contains a label in front of the display
    of the actual value.

    \sa Utils::StringAspect
*/

class Internal::FilePathAspectPrivate
{
public:
    std::function<QString(const QString &)> m_displayFilter;

    QString m_placeHolderText;
    QString m_prompDialogFilter;
    QString m_prompDialogTitle;
    QStringList m_commandVersionArguments;
    Key m_historyCompleterKey;
    PathChooser::Kind m_expectedKind = PathChooser::File;
    Environment m_environment;
    QPointer<PathChooser> m_pathChooserDisplay;
    MacroExpanderProvider m_expanderProvider;
    FilePath m_baseFileName;
    StringAspect::ValueAcceptor m_valueAcceptor;
    std::optional<FancyLineEdit::ValidationFunction> m_validator;
    std::function<void()> m_openTerminal;

    CheckableAspectImplementation m_checkerImpl;

    bool m_showToolTipOnLabel = false;
    bool m_fileDialogOnly = false;
    bool m_autoApplyOnEditingFinished = false;
    bool m_allowPathFromDevice = true;
    bool m_validatePlaceHolder = false;
};

FilePathAspect::FilePathAspect(AspectContainer *container)
    : TypedAspect(container), d(new Internal::FilePathAspectPrivate)
{
    setSpan(2, 1); // Default: Label + something

    addDataExtractor(this, &FilePathAspect::value, &Data::value);
    addDataExtractor(this, &FilePathAspect::operator(), &Data::filePath);
}

FilePathAspect::~FilePathAspect() = default;

/*!
    Returns the value of this aspect as \c Utils::FilePath.

    \note This simply uses \c FilePath::fromUserInput() for the
    conversion. It does not use any check that the value is actually
    a valid file path.
*/

FilePath FilePathAspect::operator()() const
{
    return FilePath::fromUserInput(TypedAspect::value());
}

FilePath FilePathAspect::expandedValue() const
{
    return FilePath::fromUserInput(TypedAspect::value());
}

QString FilePathAspect::value() const
{
    return TypedAspect::value();
}

/*!
    Sets the value of this file path aspect to \a value.

    \note This does not use any check that the value is actually
    a file path.
*/

void FilePathAspect::setValue(const FilePath &filePath, Announcement howToAnnounce)
{
    TypedAspect::setValue(filePath.toUserOutput(), howToAnnounce);
}

void FilePathAspect::setValue(const QString &filePath, Announcement howToAnnounce)
{
    TypedAspect::setValue(filePath, howToAnnounce);
}

void FilePathAspect::setDefaultValue(const QString &filePath)
{
    TypedAspect::setDefaultValue(filePath);
}

/*!
    Adds a check box with a \a checkerLabel according to \a checkBoxPlacement
    to the line edit.

    The state of the check box is made persistent when using a non-emtpy
    \a checkerKey.
*/
void FilePathAspect::makeCheckable(CheckBoxPlacement checkBoxPlacement,
                                   const QString &checkerLabel,
                                   const Key &checkerKey)
{
    d->m_checkerImpl.makeCheckable(checkBoxPlacement, checkerLabel, checkerKey, this);
}

bool FilePathAspect::isChecked() const
{
    return d->m_checkerImpl.isChecked();
}

void FilePathAspect::setChecked(bool checked)
{
    return d->m_checkerImpl.setChecked(checked);
}

void FilePathAspect::setValueAcceptor(ValueAcceptor &&acceptor)
{
    d->m_valueAcceptor = std::move(acceptor);
}

bool FilePathAspect::guiToBuffer()
{
    if (d->m_pathChooserDisplay)
        return updateStorage(m_buffer, d->m_pathChooserDisplay->lineEdit()->text());
    return false;
}

bool FilePathAspect::bufferToInternal()
{
    if (d->m_valueAcceptor) {
        if (const std::optional<QString> tmp = d->m_valueAcceptor(m_internal, m_buffer))
           return updateStorage(m_internal, *tmp);
    }
    return updateStorage(m_internal, m_buffer);
}

bool FilePathAspect::internalToBuffer()
{
    const QString val = d->m_displayFilter ? d->m_displayFilter(m_internal) : m_internal;
    return updateStorage(m_buffer, val);
}

void FilePathAspect::bufferToGui()
{
    if (d->m_pathChooserDisplay) {
        d->m_pathChooserDisplay->lineEdit()->setText(m_buffer);
        d->m_checkerImpl.updateWidgetFromCheckStatus(this, d->m_pathChooserDisplay.data());
    }

    validateInput();
}

PathChooser *FilePathAspect::pathChooser() const
{
    return d->m_pathChooserDisplay.data();
}

void FilePathAspect::addToLayout(Layouting::LayoutItem &parent)
{
    d->m_checkerImpl.addToLayoutFirst(parent);

    const auto useMacroExpander = [this](QWidget *w) {
        if (!d->m_expanderProvider)
            return;
        const auto chooser = new VariableChooser(w);
        chooser->addSupportedWidget(w);
        chooser->addMacroExpanderProvider(d->m_expanderProvider);
    };

    const QString displayedString = d->m_displayFilter ? d->m_displayFilter(value()) : value();

    d->m_pathChooserDisplay = createSubWidget<PathChooser>();
    d->m_pathChooserDisplay->setExpectedKind(d->m_expectedKind);
    if (!d->m_historyCompleterKey.isEmpty())
        d->m_pathChooserDisplay->setHistoryCompleter(d->m_historyCompleterKey);

    if (d->m_validator)
        d->m_pathChooserDisplay->setValidationFunction(*d->m_validator);
    d->m_pathChooserDisplay->setEnvironment(d->m_environment);
    d->m_pathChooserDisplay->setBaseDirectory(d->m_baseFileName);
    d->m_pathChooserDisplay->setOpenTerminalHandler(d->m_openTerminal);
    d->m_pathChooserDisplay->setPromptDialogFilter(d->m_prompDialogFilter);
    d->m_pathChooserDisplay->setPromptDialogTitle(d->m_prompDialogTitle);
    d->m_pathChooserDisplay->setCommandVersionArguments(d->m_commandVersionArguments);
    d->m_pathChooserDisplay->setAllowPathFromDevice(d->m_allowPathFromDevice);
    d->m_pathChooserDisplay->setReadOnly(isReadOnly());
    d->m_pathChooserDisplay->lineEdit()->setValidatePlaceHolder(d->m_validatePlaceHolder);
    if (defaultValue() == value())
        d->m_pathChooserDisplay->setDefaultValue(defaultValue());
    else
        d->m_pathChooserDisplay->setFilePath(FilePath::fromUserInput(displayedString));
    // do not override default value with placeholder, but use placeholder if default is empty
    if (d->m_pathChooserDisplay->lineEdit()->placeholderText().isEmpty())
        d->m_pathChooserDisplay->lineEdit()->setPlaceholderText(d->m_placeHolderText);
    d->m_checkerImpl.updateWidgetFromCheckStatus(this, d->m_pathChooserDisplay.data());
    addLabeledItem(parent, d->m_pathChooserDisplay);
    useMacroExpander(d->m_pathChooserDisplay->lineEdit());
    connect(d->m_pathChooserDisplay, &PathChooser::validChanged, this, &FilePathAspect::validChanged);
    bufferToGui();
    if (isAutoApply() && d->m_autoApplyOnEditingFinished) {
        connect(d->m_pathChooserDisplay, &PathChooser::editingFinished,
                this, &FilePathAspect::handleGuiChanged);
        connect(d->m_pathChooserDisplay, &PathChooser::browsingFinished,
                this, &FilePathAspect::handleGuiChanged);
    } else {
        connect(d->m_pathChooserDisplay, &PathChooser::textChanged,
                this, &FilePathAspect::handleGuiChanged);
    }

    d->m_checkerImpl.addToLayoutLast(parent);
}

/*!
    \reimp
*/
void FilePathAspect::fromMap(const Store &map)
{
    if (!settingsKey().isEmpty())
        setValue(map.value(settingsKey(), defaultValue()).toString(), BeQuiet);
    d->m_checkerImpl.fromMap(map);
}

/*!
    \reimp
*/
void FilePathAspect::toMap(Store &map) const
{
    saveToMap(map, value(), defaultValue(), settingsKey());
    d->m_checkerImpl.toMap(map);
}

void FilePathAspect::volatileToMap(Store &map) const
{
    saveToMap(map, volatileValue(), defaultValue(), settingsKey());
    d->m_checkerImpl.volatileToMap(map);
}

void FilePathAspect::setPromptDialogFilter(const QString &filter)
{
    d->m_prompDialogFilter = filter;
    if (d->m_pathChooserDisplay)
        d->m_pathChooserDisplay->setPromptDialogFilter(filter);
}

void FilePathAspect::setPromptDialogTitle(const QString &title)
{
    d->m_prompDialogTitle = title;
    if (d->m_pathChooserDisplay)
        d->m_pathChooserDisplay->setPromptDialogTitle(title);
}

void FilePathAspect::setCommandVersionArguments(const QStringList &arguments)
{
    d->m_commandVersionArguments = arguments;
    if (d->m_pathChooserDisplay)
        d->m_pathChooserDisplay->setCommandVersionArguments(arguments);
}

void FilePathAspect::setAllowPathFromDevice(bool allowPathFromDevice)
{
    d->m_allowPathFromDevice = allowPathFromDevice;
    if (d->m_pathChooserDisplay)
        d->m_pathChooserDisplay->setAllowPathFromDevice(allowPathFromDevice);
}

void FilePathAspect::setValidatePlaceHolder(bool validatePlaceHolder)
{
    d->m_validatePlaceHolder = validatePlaceHolder;
    if (d->m_pathChooserDisplay)
        d->m_pathChooserDisplay->lineEdit()->setValidatePlaceHolder(validatePlaceHolder);
}

void FilePathAspect::setShowToolTipOnLabel(bool show)
{
    d->m_showToolTipOnLabel = show;
    bufferToGui();
}

void FilePathAspect::setAutoApplyOnEditingFinished(bool applyOnEditingFinished)
{
    d->m_autoApplyOnEditingFinished = applyOnEditingFinished;
}

/*!
  Sets \a expectedKind as expected kind for path chooser displays.

  \sa Utils::PathChooser::setExpectedKind()
*/
void FilePathAspect::setExpectedKind(const PathChooser::Kind expectedKind)
{
    d->m_expectedKind = expectedKind;
    if (d->m_pathChooserDisplay)
        d->m_pathChooserDisplay->setExpectedKind(expectedKind);
}

void FilePathAspect::setEnvironment(const Environment &env)
{
    d->m_environment = env;
    if (d->m_pathChooserDisplay)
        d->m_pathChooserDisplay->setEnvironment(env);
}

void FilePathAspect::setBaseFileName(const FilePath &baseFileName)
{
    d->m_baseFileName = baseFileName;
    if (d->m_pathChooserDisplay)
        d->m_pathChooserDisplay->setBaseDirectory(baseFileName);
}

void FilePathAspect::setPlaceHolderText(const QString &placeHolderText)
{
    d->m_placeHolderText = placeHolderText;
}

void FilePathAspect::setValidationFunction(const FancyLineEdit::ValidationFunction &validator)
{
    d->m_validator = validator;
    if (d->m_pathChooserDisplay)
        d->m_pathChooserDisplay->setValidationFunction(*d->m_validator);
}

void FilePathAspect::setDisplayFilter(const std::function<QString (const QString &)> &displayFilter)
{
    d->m_displayFilter = displayFilter;
}

void FilePathAspect::setHistoryCompleter(const Key &historyCompleterKey)
{
    d->m_historyCompleterKey = historyCompleterKey;
    if (d->m_pathChooserDisplay)
        d->m_pathChooserDisplay->setHistoryCompleter(historyCompleterKey);
}

void FilePathAspect::setMacroExpanderProvider(const MacroExpanderProvider &expanderProvider)
{
    d->m_expanderProvider = expanderProvider;
}

void FilePathAspect::validateInput()
{
    if (d->m_pathChooserDisplay)
        d->m_pathChooserDisplay->triggerChanged();
}

void FilePathAspect::setOpenTerminalHandler(const std::function<void ()> &openTerminal)
{
    d->m_openTerminal = openTerminal;
    if (d->m_pathChooserDisplay)
        d->m_pathChooserDisplay->setOpenTerminalHandler(openTerminal);
}

/*!
    \class Utils::ColorAspect
    \inmodule QtCreator

    \brief A color aspect is a color property of some object, together with
    a description of its behavior for common operations like visualizing or
    persisting.

    The color aspect is displayed using a QtColorButton.
*/

ColorAspect::ColorAspect(AspectContainer *container)
    : TypedAspect(container), d(new Internal::ColorAspectPrivate)
{
    setDefaultValue(QColor::fromRgb(0, 0, 0));
    setSpan(1, 1);
}

ColorAspect::~ColorAspect() = default;

void ColorAspect::addToLayout(Layouting::LayoutItem &parent)
{
    QTC_CHECK(!d->m_colorButton);
    d->m_colorButton = createSubWidget<QtColorButton>();
    parent.addItem(d->m_colorButton.data());

    bufferToGui();
    connect(d->m_colorButton.data(), &QtColorButton::colorChanged,
            this, &ColorAspect::handleGuiChanged);
}

bool ColorAspect::guiToBuffer()
{
    if (d->m_colorButton)
        return updateStorage(m_buffer, d->m_colorButton->color());
    return false;
}

void ColorAspect::bufferToGui()
{
    if (d->m_colorButton)
        d->m_colorButton->setColor(m_buffer);
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


BoolAspect::BoolAspect(AspectContainer *container)
    : TypedAspect(container), d(new Internal::BoolAspectPrivate)
{
    setDefaultValue(false);
    setSpan(2, 1);

    d->m_undoable.setSilently(false);
}

/*!
    \internal
*/
BoolAspect::~BoolAspect() = default;

void BoolAspect::addToLayoutHelper(Layouting::LayoutItem &parent, QAbstractButton *button)
{
    switch (d->m_labelPlacement) {
    case LabelPlacement::Compact:
        button->setText(labelText());
        parent.addItem(button);
        break;
    case LabelPlacement::AtCheckBox:
        button->setText(labelText());
        parent.addItem(empty());
        parent.addItem(button);
        break;
    case LabelPlacement::InExtraLabel:
        addLabeledItem(parent, button);
        break;
    }

    connect(button, &QAbstractButton::clicked, this, [button, this] {
        d->m_undoable.set(undoStack(), button->isChecked());
    });

    connect(&d->m_undoable.m_signal, &UndoSignaller::changed, button, [button, this] {
        button->setChecked(d->m_undoable.get());
        handleGuiChanged();
    });
}

LayoutItem BoolAspect::adoptButton(QAbstractButton *button)
{
    LayoutItem parent;

    addToLayoutHelper(parent, button);

    bufferToGui();
    return parent;
}

/*!
    \reimp
*/
void BoolAspect::addToLayout(Layouting::LayoutItem &parent)
{
    QCheckBox *checkBox = createSubWidget<QCheckBox>();
    addToLayoutHelper(parent, checkBox);
    bufferToGui();
}

std::function<void (QObject *)> BoolAspect::groupChecker()
{
    return [this](QObject *target) {
        auto groupBox = qobject_cast<QGroupBox *>(target);
        QTC_ASSERT(groupBox, return);
        registerSubWidget(groupBox);
        groupBox->setCheckable(true);
        groupBox->setChecked(value());

        connect(groupBox, &QGroupBox::clicked, this, [groupBox, this] {
            d->m_undoable.set(undoStack(), groupBox->isChecked());
        });

        connect(&d->m_undoable.m_signal, &UndoSignaller::changed, groupBox, [groupBox, this] {
            groupBox->setChecked(d->m_undoable.get());
            handleGuiChanged();
        });
        bufferToGui();
    };
}

QAction *BoolAspect::action()
{
    if (hasAction())
        return TypedAspect::action();
    auto act = TypedAspect::action(); // Creates it.
    act->setCheckable(true);
    act->setChecked(m_internal);
    act->setToolTip(toolTip());
    connect(act, &QAction::triggered, this, [this](bool newValue) {
        setValue(newValue);
    });
    connect(this, &BoolAspect::changed, act, [act, this] { act->setChecked(m_internal); });

    return act;
}

bool BoolAspect::guiToBuffer()
{
    return updateStorage(m_buffer, d->m_undoable.get());
}

void BoolAspect::bufferToGui()
{
    d->m_undoable.setWithoutUndo(m_buffer);
}

void BoolAspect::setLabel(const QString &labelText, LabelPlacement labelPlacement)
{
    TypedAspect::setLabelText(labelText);
    d->m_labelPlacement = labelPlacement;
}

void BoolAspect::setLabelPlacement(BoolAspect::LabelPlacement labelPlacement)
{
    d->m_labelPlacement = labelPlacement;
}

CheckableDecider BoolAspect::askAgainCheckableDecider()
{
    return CheckableDecider(
        [this] { return value(); },
        [this] { setValue(false); }
    );
}

CheckableDecider BoolAspect::doNotAskAgainCheckableDecider()
{
    return CheckableDecider(
        [this] { return !value(); },
        [this] { setValue(true); }
    );
}

/*!
    \class Utils::SelectionAspect
    \inmodule QtCreator

    \brief A selection aspect represents a specific choice out of
    several.

    The selection aspect is displayed using a QComboBox or
    QRadioButtons in a QButtonGroup.
*/

SelectionAspect::SelectionAspect(AspectContainer *container)
    : TypedAspect(container), d(new Internal::SelectionAspectPrivate)
{
    setSpan(2, 1);
}

/*!
    \internal
*/
SelectionAspect::~SelectionAspect() = default;

/*!
    \reimp
*/
void SelectionAspect::addToLayout(Layouting::LayoutItem &parent)
{
    QTC_CHECK(d->m_buttonGroup == nullptr);
    QTC_CHECK(!d->m_comboBox);
    QTC_ASSERT(d->m_buttons.isEmpty(), d->m_buttons.clear());

    switch (d->m_displayStyle) {
    case DisplayStyle::RadioButtons:
        d->m_buttonGroup = new QButtonGroup();
        d->m_buttonGroup->setExclusive(true);
        for (int i = 0, n = d->m_options.size(); i < n; ++i) {
            const Option &option = d->m_options.at(i);
            auto button = createSubWidget<QRadioButton>(option.displayName);
            button->setChecked(i == value());
            button->setEnabled(option.enabled);
            button->setToolTip(option.tooltip);
            parent.addItem(button);
            d->m_buttons.append(button);
            d->m_buttonGroup->addButton(button, i);
        }
        bufferToGui();
        connect(d->m_buttonGroup, &QButtonGroup::idClicked,
                this, &SelectionAspect::handleGuiChanged);
        break;
    case DisplayStyle::ComboBox:
        setLabelText(displayName());
        d->m_comboBox = createSubWidget<QComboBox>();
        for (int i = 0, n = d->m_options.size(); i < n; ++i)
            d->m_comboBox->addItem(d->m_options.at(i).displayName);
        d->m_comboBox->setCurrentIndex(value());
        addLabeledItem(parent, d->m_comboBox);
        bufferToGui();
        connect(d->m_comboBox.data(), &QComboBox::activated,
                this, &SelectionAspect::handleGuiChanged);
        break;
    }
}

bool SelectionAspect::guiToBuffer()
{
    const int old = m_buffer;
    switch (d->m_displayStyle) {
    case DisplayStyle::RadioButtons:
        if (d->m_buttonGroup)
            m_buffer = d->m_buttonGroup->checkedId();
        break;
    case DisplayStyle::ComboBox:
        if (d->m_comboBox)
            m_buffer = d->m_comboBox->currentIndex();
        break;
    }
    return m_buffer != old;
}

void SelectionAspect::bufferToGui()
{
    if (d->m_buttonGroup) {
        QAbstractButton *button = d->m_buttonGroup->button(m_buffer);
        QTC_ASSERT(button, return);
        button->setChecked(true);
    } else if (d->m_comboBox) {
        d->m_comboBox->setCurrentIndex(m_buffer);
    }
}

void SelectionAspect::finish()
{
    delete d->m_buttonGroup;
    d->m_buttonGroup = nullptr;
    BaseAspect::finish();
    d->m_buttons.clear();
}

void SelectionAspect::setDisplayStyle(SelectionAspect::DisplayStyle style)
{
    d->m_displayStyle = style;
}

void SelectionAspect::setStringValue(const QString &val)
{
    const int index = indexForDisplay(val);
    QTC_ASSERT(index >= 0, return);
    setValue(index);
}

void SelectionAspect::setDefaultValue(int val)
{
    TypedAspect::setDefaultValue(val);
}

// Note: This needs to be set after all options are added.
void SelectionAspect::setDefaultValue(const QString &val)
{
    TypedAspect::setDefaultValue(indexForDisplay(val));
}

QString SelectionAspect::stringValue() const
{
    const int idx = value();
    return idx >= 0 && idx < d->m_options.size() ? d->m_options.at(idx).displayName : QString();
}

QVariant SelectionAspect::itemValue() const
{
    const int idx = value();
    return idx >= 0 && idx < d->m_options.size() ? d->m_options.at(idx).itemData : QVariant();
}

void SelectionAspect::addOption(const QString &displayName, const QString &toolTip)
{
    d->m_options.append(Option(displayName, toolTip, {}));
}

void SelectionAspect::addOption(const Option &option)
{
    d->m_options.append(option);
}

int SelectionAspect::indexForDisplay(const QString &displayName) const
{
    for (int i = 0, n = d->m_options.size(); i < n; ++i) {
        if (d->m_options.at(i).displayName == displayName)
            return i;
    }
    return -1;
}

QString SelectionAspect::displayForIndex(int index) const
{
    QTC_ASSERT(index >= 0 && index < d->m_options.size(), return {});
    return d->m_options.at(index).displayName;
}

int SelectionAspect::indexForItemValue(const QVariant &value) const
{
    for (int i = 0, n = d->m_options.size(); i < n; ++i) {
        if (d->m_options.at(i).itemData == value)
            return i;
    }
    return -1;
}

QVariant SelectionAspect::itemValueForIndex(int index) const
{
    QTC_ASSERT(index >= 0 && index < d->m_options.size(), return {});
    return d->m_options.at(index).itemData;
}

/*!
    \class Utils::MultiSelectionAspect
    \inmodule QtCreator

    \brief A multi-selection aspect represents one or more choices out of
    several.

    The multi-selection aspect is displayed using a QListWidget with
    checkable items.
*/

MultiSelectionAspect::MultiSelectionAspect(AspectContainer *container)
    : TypedAspect(container), d(new Internal::MultiSelectionAspectPrivate(this))
{
    setDefaultValue(QStringList());
    setSpan(2, 1);
}

/*!
    \internal
*/
MultiSelectionAspect::~MultiSelectionAspect() = default;

/*!
    \reimp
*/
void MultiSelectionAspect::addToLayout(LayoutItem &builder)
{
    QTC_CHECK(d->m_listView == nullptr);
    if (d->m_allValues.isEmpty())
        return;

    switch (d->m_displayStyle) {
    case DisplayStyle::ListView:
        d->m_listView = createSubWidget<QListWidget>();
        for (const QString &val : std::as_const(d->m_allValues))
            (void) new QListWidgetItem(val, d->m_listView);
        addLabeledItem(builder, d->m_listView);

        bufferToGui();
        connect(d->m_listView, &QListWidget::itemChanged,
                this, &MultiSelectionAspect::handleGuiChanged);
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

void MultiSelectionAspect::setDisplayStyle(MultiSelectionAspect::DisplayStyle style)
{
    d->m_displayStyle = style;
}

void MultiSelectionAspect::bufferToGui()
{
    if (d->m_listView) {
        const int n = d->m_listView->count();
        QTC_CHECK(n == d->m_allValues.size());
        for (int i = 0; i != n; ++i) {
            auto item = d->m_listView->item(i);
            item->setCheckState(m_buffer.contains(item->text()) ? Qt::Checked : Qt::Unchecked);
        }
    }
}

bool MultiSelectionAspect::guiToBuffer()
{
    if (d->m_listView) {
        QStringList val;
        const int n = d->m_listView->count();
        QTC_CHECK(n == d->m_allValues.size());
        for (int i = 0; i != n; ++i) {
            auto item = d->m_listView->item(i);
            if (item->checkState() == Qt::Checked)
                val.append(item->text());
        }
        return updateStorage(m_buffer, val);
    }
    return false;
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

IntegerAspect::IntegerAspect(AspectContainer *container)
    : TypedAspect(container), d(new Internal::IntegerAspectPrivate)
{
    setSpan(2, 1);
}

/*!
    \internal
*/
IntegerAspect::~IntegerAspect() = default;

/*!
    \reimp
*/
void IntegerAspect::addToLayout(Layouting::LayoutItem &parent)
{
    QTC_CHECK(!d->m_spinBox);
    d->m_spinBox = createSubWidget<QSpinBox>();
    d->m_spinBox->setDisplayIntegerBase(d->m_displayIntegerBase);
    d->m_spinBox->setPrefix(d->m_prefix);
    d->m_spinBox->setSuffix(d->m_suffix);
    d->m_spinBox->setSingleStep(d->m_singleStep);
    d->m_spinBox->setSpecialValueText(d->m_specialValueText);
    if (d->m_maximumValue && d->m_maximumValue)
        d->m_spinBox->setRange(int(d->m_minimumValue.value() / d->m_displayScaleFactor),
                               int(d->m_maximumValue.value() / d->m_displayScaleFactor));
    d->m_spinBox->setValue(int(value() / d->m_displayScaleFactor)); // Must happen after setRange()
    addLabeledItem(parent, d->m_spinBox);
    connect(d->m_spinBox.data(), &QSpinBox::valueChanged,
            this, &IntegerAspect::handleGuiChanged);
}

bool IntegerAspect::guiToBuffer()
{
    if (d->m_spinBox)
        return updateStorage(m_buffer, d->m_spinBox->value() * d->m_displayScaleFactor);
    return false;
}

void IntegerAspect::bufferToGui()
{
    if (d->m_spinBox)
        d->m_spinBox->setValue(m_buffer / d->m_displayScaleFactor);
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

void IntegerAspect::setSpecialValueText(const QString &specialText)
{
    d->m_specialValueText = specialText;
}

void IntegerAspect::setSingleStep(qint64 step)
{
    d->m_singleStep = step;
}


/*!
    \class Utils::DoubleAspect
    \inmodule QtCreator

    \brief An double aspect is a numerical property of some object, together with
    a description of its behavior for common operations like visualizing or
    persisting.

    The double aspect is displayed using a \c QDoubleSpinBox.

    The visual representation often contains a label in front
    the display of the spin box.
*/

DoubleAspect::DoubleAspect(AspectContainer *container)
    : TypedAspect(container), d(new Internal::DoubleAspectPrivate)
{
    setDefaultValue(double(0));
    setSpan(2, 1);
}

/*!
    \internal
*/
DoubleAspect::~DoubleAspect() = default;

/*!
    \reimp
*/
void DoubleAspect::addToLayout(LayoutItem &builder)
{
    QTC_CHECK(!d->m_spinBox);
    d->m_spinBox = createSubWidget<QDoubleSpinBox>();
    d->m_spinBox->setPrefix(d->m_prefix);
    d->m_spinBox->setSuffix(d->m_suffix);
    d->m_spinBox->setSingleStep(d->m_singleStep);
    d->m_spinBox->setSpecialValueText(d->m_specialValueText);
    if (d->m_maximumValue && d->m_maximumValue)
        d->m_spinBox->setRange(d->m_minimumValue.value(), d->m_maximumValue.value());
    bufferToGui(); // Must happen after setRange()!
    addLabeledItem(builder, d->m_spinBox);
    connect(d->m_spinBox.data(), &QDoubleSpinBox::valueChanged,
            this, &DoubleAspect::handleGuiChanged);
}

bool DoubleAspect::guiToBuffer()
{
    if (d->m_spinBox)
        return updateStorage(m_buffer, d->m_spinBox->value());
    return false;
}

void DoubleAspect::bufferToGui()
{
    if (d->m_spinBox)
        d->m_spinBox->setValue(m_buffer);
}

void DoubleAspect::setRange(double min, double max)
{
    d->m_minimumValue = min;
    d->m_maximumValue = max;
}

void DoubleAspect::setPrefix(const QString &prefix)
{
    d->m_prefix = prefix;
}

void DoubleAspect::setSuffix(const QString &suffix)
{
    d->m_suffix = suffix;
}

void DoubleAspect::setSpecialValueText(const QString &specialText)
{
    d->m_specialValueText = specialText;
}

void DoubleAspect::setSingleStep(double step)
{
    d->m_singleStep = step;
}


/*!
    \class Utils::TriStateAspect
    \inmodule QtCreator

    \brief A tristate aspect is a property of some object that can have
    three values: enabled, disabled, and unspecified.

    Its visual representation is a QComboBox with three items.
*/

TriStateAspect::TriStateAspect(AspectContainer *container,
                               const QString &onString,
                               const QString &offString,
                               const QString &defaultString)
    : SelectionAspect(container)
{
    setDisplayStyle(DisplayStyle::ComboBox);
    setDefaultValue(TriState::Default);
    addOption(onString.isEmpty() ? Tr::tr("Enable") : onString);
    addOption(offString.isEmpty() ? Tr::tr("Disable") : offString);
    addOption(defaultString.isEmpty() ? Tr::tr("Leave at Default") : defaultString);
}

TriState TriStateAspect::value() const
{
    return TriState::fromInt(SelectionAspect::value());
}

void TriStateAspect::setValue(TriState value)
{
    SelectionAspect::setValue(value.toInt());
}

TriState TriStateAspect::defaultValue() const
{
    return TriState::fromInt(SelectionAspect::defaultValue());
}

void TriStateAspect::setDefaultValue(TriState value)
{
    SelectionAspect::setDefaultValue(value.toInt());
}

const TriState TriState::Enabled{TriState::EnabledValue};
const TriState TriState::Disabled{TriState::DisabledValue};
const TriState TriState::Default{TriState::DefaultValue};

TriState TriState::fromVariant(const QVariant &variant)
{
    return fromInt(variant.toInt());
}

TriState TriState::fromInt(int v)
{
    QTC_ASSERT(v == EnabledValue || v == DisabledValue || v == DefaultValue, v = DefaultValue);
    return TriState(Value(v));
}


/*!
    \class Utils::StringListAspect
    \inmodule QtCreator

    \brief A string list aspect represents a property of some object
    that is a list of strings.
*/

StringListAspect::StringListAspect(AspectContainer *container)
    : TypedAspect(container), d(new Internal::StringListAspectPrivate)
{
    setDefaultValue(QStringList());
}

/*!
    \internal
*/
StringListAspect::~StringListAspect() = default;

/*!
    \reimp
*/
void StringListAspect::addToLayout(LayoutItem &parent)
{
    Q_UNUSED(parent)
    // TODO - when needed.
}

void StringListAspect::appendValue(const QString &s, bool allowDuplicates)
{
    QStringList val = value();
    if (allowDuplicates || !val.contains(s))
        val.append(s);
    setValue(val);
}

void StringListAspect::removeValue(const QString &s)
{
    QStringList val = value();
    val.removeAll(s);
    setValue(val);
}

void StringListAspect::appendValues(const QStringList &values, bool allowDuplicates)
{
    QStringList val = value();
    for (const QString &s : values) {
        if (allowDuplicates || !val.contains(s))
            val.append(s);
    }
    setValue(val);
}

void StringListAspect::removeValues(const QStringList &values)
{
    QStringList val = value();
    for (const QString &s : values)
        val.removeAll(s);
    setValue(val);
}

/*!
    \class Utils::FilePathListAspect
    \inmodule QtCreator

    \brief A filepath list aspect represents a property of some object
    that is a list of filepathList.
*/

FilePathListAspect::FilePathListAspect(AspectContainer *container)
    : TypedAspect(container)
    , d(new Internal::FilePathListAspectPrivate)
{
    setDefaultValue(QStringList());
}

FilePathListAspect::~FilePathListAspect() = default;

FilePaths FilePathListAspect::operator()() const
{
    return Utils::transform(m_internal, &FilePath::fromUserInput);
}

bool FilePathListAspect::guiToBuffer()
{
    const QStringList newValue = d->undoable.get();
    if (newValue != m_buffer) {
        m_buffer = newValue;
        return true;
    }
    return false;
}

void FilePathListAspect::bufferToGui()
{
    d->undoable.setWithoutUndo(m_buffer);
}

void FilePathListAspect::addToLayout(LayoutItem &parent)
{
    d->undoable.setSilently(value());

    PathListEditor *editor = new PathListEditor;
    editor->setPathList(value());
    connect(editor, &PathListEditor::changed, this, [this, editor] {
        d->undoable.set(undoStack(), editor->pathList());
    });
    connect(&d->undoable.m_signal, &UndoSignaller::changed, editor, [this, editor] {
        if (editor->pathList() != d->undoable.get())
            editor->setPathList(d->undoable.get());

        handleGuiChanged();
    });

    editor->setToolTip(toolTip());
    editor->setMaximumHeight(100);
    editor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    editor->setPlaceholderText(d->placeHolderText);

    registerSubWidget(editor);

    parent.addItem(editor);
}

void FilePathListAspect::setPlaceHolderText(const QString &placeHolderText)
{
    d->placeHolderText = placeHolderText;

    forEachSubWidget([placeHolderText](QWidget *widget) {
        if (auto pathListEditor = qobject_cast<PathListEditor *>(widget)) {
            pathListEditor->setPlaceholderText(placeHolderText);
        }
    });
}

void FilePathListAspect::appendValue(const FilePath &path, bool allowDuplicates)
{
    const QString asString = path.toUserOutput();
    QStringList val = value();
    if (allowDuplicates || !val.contains(asString))
        val.append(asString);
    setValue(val);
}

void FilePathListAspect::removeValue(const FilePath &s)
{
    QStringList val = value();
    val.removeAll(s.toUserOutput());
    setValue(val);
}

void FilePathListAspect::appendValues(const FilePaths &paths, bool allowDuplicates)
{
    QStringList val = value();

    for (const FilePath &path : paths) {
        const QString asString = path.toUserOutput();
        if (allowDuplicates || !val.contains(asString))
            val.append(asString);
    }
    setValue(val);
}

void FilePathListAspect::removeValues(const FilePaths &paths)
{
    QStringList val = value();
    for (const FilePath &path : paths)
        val.removeAll(path.toUserOutput());
    setValue(val);
}

/*!
    \class Utils::IntegerListAspect
    \internal
    \inmodule QtCreator

    \brief A string list aspect represents a property of some object
    that is a list of strings.
*/

IntegersAspect::IntegersAspect(AspectContainer *container)
    : TypedAspect(container)
{}

/*!
    \internal
*/
IntegersAspect::~IntegersAspect() = default;

/*!
    \reimp
*/
void IntegersAspect::addToLayout(Layouting::LayoutItem &parent)
{
    Q_UNUSED(parent)
    // TODO - when needed.
}


/*!
    \class Utils::TextDisplay
    \inmodule QtCreator

    \brief A text display is a phony aspect with the sole purpose of providing
    some text display using an Utils::InfoLabel in places where otherwise
    more expensive Utils::StringAspect items would be used.

    A text display does not have a real value.
*/

/*!
    Constructs a text display showing the \a message with an icon representing
    type \a type.
 */
TextDisplay::TextDisplay(AspectContainer *container, const QString &message, InfoLabel::InfoType type)
    : BaseAspect(container), d(new Internal::TextDisplayPrivate)
{
    d->m_message = message;
    d->m_type = type;
}

/*!
    \internal
*/
TextDisplay::~TextDisplay() = default;

/*!
    \reimp
*/
void TextDisplay::addToLayout(LayoutItem &parent)
{
    if (!d->m_label) {
        d->m_label = createSubWidget<InfoLabel>(d->m_message, d->m_type);
        d->m_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
        d->m_label->setElideMode(Qt::ElideNone);
        d->m_label->setWordWrap(true);
        // Do not use m_label->setVisible(isVisible()) unconditionally, it does not
        // have a QWidget parent yet when used in a LayoutBuilder.
        if (!isVisible())
            d->m_label->setVisible(false);

        connect(this, &TextDisplay::changed, d->m_label, [this] {
            d->m_label->setText(d->m_message);
        });
    }
    parent.addItem(d->m_label.data());
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

void TextDisplay::setText(const QString &message)
{
    d->m_message = message;
    emit changed();
}

/*!
    \class Utils::AspectContainer
    \inmodule QtCreator

    \brief The AspectContainer class wraps one or more aspects while providing
    the interface of a single aspect.

    Sub-aspects ownership can be declared using \a setOwnsSubAspects.
*/

class Internal::AspectContainerPrivate
{
public:
    QList<BaseAspect *> m_items; // Both owned and non-owned.
    QList<BaseAspect *> m_ownedItems; // Owned only.
    QStringList m_settingsGroup;
    std::function<Layouting::LayoutItem ()> m_layouter;
};

AspectContainer::AspectContainer()
    : d(new Internal::AspectContainerPrivate)
{}

/*!
    \internal
*/
AspectContainer::~AspectContainer()
{
    qDeleteAll(d->m_ownedItems);
}

/*!
    \internal
*/
void AspectContainer::registerAspect(BaseAspect *aspect, bool takeOwnership)
{
    aspect->setAutoApply(isAutoApply());
    d->m_items.append(aspect);
    if (takeOwnership)
        d->m_ownedItems.append(aspect);
}

void AspectContainer::registerAspects(const AspectContainer &aspects)
{
    for (BaseAspect *aspect : std::as_const(aspects.d->m_items))
        registerAspect(aspect);
}

/*!
    Retrieves a BaseAspect with a given \a id, or nullptr if no such aspect is contained.

    \sa BaseAspect
*/
BaseAspect *AspectContainer::aspect(Id id) const
{
    return findOrDefault(d->m_items, equal(&BaseAspect::id, id));
}

AspectContainer::const_iterator AspectContainer::begin() const
{
    return d->m_items.cbegin();
}

AspectContainer::const_iterator AspectContainer::end() const
{
    return d->m_items.cend();
}

void AspectContainer::setLayouter(const std::function<Layouting::LayoutItem ()> &layouter)
{
    d->m_layouter = layouter;
}

std::function<LayoutItem ()> AspectContainer::layouter() const
{
    return d->m_layouter;
}

const QList<BaseAspect *> &AspectContainer::aspects() const
{
    return d->m_items;
}

void AspectContainer::fromMap(const Store &map)
{
    for (BaseAspect *aspect : std::as_const(d->m_items))
        aspect->fromMap(map);

    emit fromMapFinished();

}

void AspectContainer::toMap(Store &map) const
{
    for (BaseAspect *aspect : std::as_const(d->m_items))
        aspect->toMap(map);
}

void AspectContainer::volatileToMap(Store &map) const
{
    for (BaseAspect *aspect : std::as_const(d->m_items))
        aspect->volatileToMap(map);
}

void AspectContainer::readSettings()
{
    const SettingsGroupNester nester(d->m_settingsGroup);
    for (BaseAspect *aspect : std::as_const(d->m_items))
        aspect->readSettings();
}

void AspectContainer::writeSettings() const
{
    const SettingsGroupNester nester(d->m_settingsGroup);
    for (BaseAspect *aspect : std::as_const(d->m_items))
        aspect->writeSettings();
}

void AspectContainer::setSettingsGroup(const QString &groupKey)
{
    d->m_settingsGroup = QStringList{groupKey};
}

void AspectContainer::setSettingsGroups(const QString &groupKey, const QString &subGroupKey)
{
    d->m_settingsGroup = QStringList{groupKey, subGroupKey};
}

QStringList AspectContainer::settingsGroups() const
{
    return d->m_settingsGroup;
}

void AspectContainer::apply()
{
    const bool willChange = isDirty();

    for (BaseAspect *aspect : std::as_const(d->m_items))
        aspect->apply();

    emit applied();

    if (willChange)
        emit changed();
}

void AspectContainer::cancel()
{
    for (BaseAspect *aspect : std::as_const(d->m_items))
        aspect->cancel();
}

void AspectContainer::finish()
{
    for (BaseAspect *aspect : std::as_const(d->m_items))
        aspect->finish();
}

void AspectContainer::reset()
{
    for (BaseAspect *aspect : std::as_const(d->m_items))
        aspect->setVariantValue(aspect->defaultVariantValue());
}

void AspectContainer::setAutoApply(bool on)
{
    BaseAspect::setAutoApply(on);

    for (BaseAspect *aspect : std::as_const(d->m_items))
        aspect->setAutoApply(on);
}

bool AspectContainer::isDirty()
{
    for (BaseAspect *aspect : std::as_const(d->m_items)) {
        if (aspect->isDirty())
            return true;
    }
    return false;
}

void AspectContainer::setUndoStack(QUndoStack *undoStack)
{
    BaseAspect::setUndoStack(undoStack);

    for (BaseAspect *aspect : std::as_const(d->m_items))
        aspect->setUndoStack(undoStack);
}

bool AspectContainer::equals(const AspectContainer &other) const
{
    // FIXME: Expensive, but should not really be needed in a fully aspectified world.
    Store thisMap, thatMap;
    toMap(thisMap);
    other.toMap(thatMap);
    return thisMap == thatMap;
}

void AspectContainer::copyFrom(const AspectContainer &other)
{
    Store map;
    other.toMap(map);
    fromMap(map);
}

void AspectContainer::forEachAspect(const std::function<void(BaseAspect *)> &run) const
{
    for (BaseAspect *aspect : std::as_const(d->m_items)) {
        if (auto container = dynamic_cast<AspectContainer *>(aspect))
            container->forEachAspect(run);
        else
            run(aspect);
    }
}

BaseAspect::Data::Ptr BaseAspect::extractData() const
{
    QTC_ASSERT(d->m_dataCreator, return {});
    Data *data = d->m_dataCreator();
    data->m_classId = metaObject();
    data->m_id = id();
    data->m_cloner = d->m_dataCloner;
    for (const DataExtractor &extractor : d->m_dataExtractors)
        extractor(data);
    return Data::Ptr(data);
}

/*
    Mirrors the internal volatile value to the GUI element if they are already
    created.

    No-op otherwise.
*/
void BaseAspect::bufferToGui()
{
}

/*
    Mirrors the data stored in GUI element if they are already created to
    the internal volatile value.

    No-op otherwise.

    \return true when the buffered volatile value changed.
*/
bool BaseAspect::guiToBuffer()
{
    return false;
}

/*
    Mirrors buffered volatile value to the internal value.
    This function is used for \c apply().

    \return true when the internal value changed.
*/

bool BaseAspect::bufferToInternal()
{
    return false;
}


bool BaseAspect::internalToBuffer()
{
    return false;
}

void BaseAspect::handleGuiChanged()
{
    if (guiToBuffer())
        volatileValueChanged();
    if (isAutoApply())
        apply();
}

void BaseAspect::addDataExtractorHelper(const DataExtractor &extractor) const
{
    d->m_dataExtractors.append(extractor);
}

void BaseAspect::setDataCreatorHelper(const DataCreator &creator) const
{
    d->m_dataCreator = creator;
}

void BaseAspect::setDataClonerHelper(const DataCloner &cloner) const
{
    d->m_dataCloner = cloner;
}

const BaseAspect::Data *AspectContainerData::aspect(Id instanceId) const
{
    for (const BaseAspect::Data::Ptr &data : m_data) {
        if (data.get()->id() == instanceId)
            return data.get();
    }
    return nullptr;
}

const BaseAspect::Data *AspectContainerData::aspect(BaseAspect::Data::ClassId classId) const
{
    for (const BaseAspect::Data::Ptr &data : m_data) {
        if (data.get()->classId() == classId)
            return data.get();
    }
    return nullptr;
}

void AspectContainerData::append(const BaseAspect::Data::Ptr &data)
{
    m_data.append(data);
}

// BaseAspect::Data

BaseAspect::Data::~Data() = default;

void BaseAspect::Data::Ptr::operator=(const Ptr &other)
{
    if (this == &other)
        return;
    delete m_data;
    m_data = other.m_data->clone();
}

// SettingsGroupNester

SettingsGroupNester::SettingsGroupNester(const QStringList &groups)
    : m_groupCount(groups.size())
{
    QTC_ASSERT(theSettings, return);
    for (const QString &group : groups)
        theSettings->beginGroup(keyFromString(group));
}

SettingsGroupNester::~SettingsGroupNester()
{
    QTC_ASSERT(theSettings, return);
    for (int i = 0; i != m_groupCount; ++i)
        theSettings->endGroup();
}

class AddItemCommand : public QUndoCommand
{
public:
    AddItemCommand(AspectList *aspect, const std::shared_ptr<BaseAspect> &item)
        : m_aspect(aspect)
        , m_item(item)
    {}

    void undo() override { m_aspect->actualRemoveItem(m_item); }
    void redo() override { m_aspect->actualAddItem(m_item); }

private:
    AspectList *m_aspect;
    std::shared_ptr<BaseAspect> m_item;
};

class RemoveItemCommand : public QUndoCommand
{
public:
    RemoveItemCommand(AspectList *aspect, const std::shared_ptr<BaseAspect> &item)
        : m_aspect(aspect)
        , m_item(item)
    {}

    void undo() override { m_aspect->actualAddItem(m_item); }
    void redo() override { m_aspect->actualRemoveItem(m_item); }

private:
    AspectList *m_aspect;
    std::shared_ptr<BaseAspect> m_item;
};

class Internal::AspectListPrivate
{
public:
    QList<std::shared_ptr<BaseAspect>> items;
    QList<std::shared_ptr<BaseAspect>> volatileItems;
    AspectList::CreateItem createItem;
    AspectList::ItemCallback itemAdded;
    AspectList::ItemCallback itemRemoved;
};

AspectList::AspectList(Utils::AspectContainer *container)
    : Utils::BaseAspect(container)
    , d(std::make_unique<Internal::AspectListPrivate>())
{}

AspectList::~AspectList() = default;

void AspectList::fromMap(const Utils::Store &map)
{
    QTC_ASSERT(!settingsKey().isEmpty(), return);

    QVariantList list = map[settingsKey()].toList();
    d->volatileItems.clear();
    for (const QVariant &entry : list) {
        auto item = d->createItem();
        item->setAutoApply(isAutoApply());
        item->setUndoStack(undoStack());
        item->fromMap(Utils::storeFromVariant(entry));
        d->volatileItems.append(item);
    }
    d->items = d->volatileItems;
}

QVariantList AspectList::toList(bool v) const
{
    QVariantList list;
    const auto &items = v ? d->volatileItems : d->items;

    for (const std::shared_ptr<BaseAspect> &item : items) {
        Utils::Store childStore;
        if (v)
            item->volatileToMap(childStore);
        else
            item->toMap(childStore);

        list.append(Utils::variantFromStore(childStore));
    }

    return list;
}

void AspectList::toMap(Utils::Store &map) const
{
    QTC_ASSERT(!settingsKey().isEmpty(), return);
    const Utils::Key key = settingsKey();
    map[key] = toList(false);
}

void AspectList::volatileToMap(Utils::Store &map) const
{
    QTC_ASSERT(!settingsKey().isEmpty(), return);
    const Utils::Key key = settingsKey();
    map[key] = toList(true);
}

std::shared_ptr<BaseAspect> AspectList::actualAddItem(const std::shared_ptr<BaseAspect> &item)
{
    item->setAutoApply(isAutoApply());
    item->setUndoStack(undoStack());

    d->volatileItems.append(item);
    if (d->itemAdded)
        d->itemAdded(item);
    emit volatileValueChanged();
    if (isAutoApply())
        d->items = d->volatileItems;
    return item;
}

QList<std::shared_ptr<BaseAspect>> AspectList::items() const
{
    return d->items;
}
QList<std::shared_ptr<BaseAspect>> AspectList::volatileItems() const
{
    return d->volatileItems;
}

std::shared_ptr<BaseAspect> AspectList::createAndAddItem()
{
    return addItem(d->createItem());
}

std::shared_ptr<BaseAspect> AspectList::addItem(const std::shared_ptr<BaseAspect> &item)
{
    if (undoStack())
        undoStack()->push(new AddItemCommand(this, item));
    else
        return actualAddItem(item);

    return item;
}

void AspectList::actualRemoveItem(const std::shared_ptr<BaseAspect> &item)
{
    d->volatileItems.removeOne(item);
    if (d->itemRemoved)
        d->itemRemoved(item);
    emit volatileValueChanged();
    if (isAutoApply())
        d->items = d->volatileItems;
}

void AspectList::removeItem(const std::shared_ptr<BaseAspect> &item)
{
    if (undoStack())
        undoStack()->push(new RemoveItemCommand(this, item));
    else
        actualRemoveItem(item);
}

void AspectList::clear()
{
    if (undoStack()) {
        undoStack()->beginMacro("Clear");

        for (const std::shared_ptr<BaseAspect> &item : volatileItems())
            undoStack()->push(new RemoveItemCommand(this, item));

        undoStack()->endMacro();
    } else {
        for (const std::shared_ptr<BaseAspect> &item : volatileItems())
            actualRemoveItem(item);
    }
}

void AspectList::apply()
{
    d->items = d->volatileItems;
    forEachItem<BaseAspect>([](const std::shared_ptr<BaseAspect> &aspect) { aspect->apply(); });
    emit changed();
}

void AspectList::setCreateItemFunction(CreateItem createItem)
{
    d->createItem = createItem;
}

void AspectList::setItemAddedCallback(const ItemCallback &callback)
{
    d->itemAdded = callback;
}
void AspectList::setItemRemovedCallback(const ItemCallback &callback)
{
    d->itemRemoved = callback;
}

qsizetype AspectList::size() const
{
    return d->volatileItems.size();
}

bool AspectList::isDirty()
{
    if (d->items != d->volatileItems)
        return true;

    for (const std::shared_ptr<BaseAspect> &item : d->volatileItems) {
        if (item->isDirty())
            return true;
    }
    return false;
}

class ColoredRow : public QWidget
{
public:
    ColoredRow(int idx, QWidget *parent = nullptr)
        : QWidget(parent)
        , m_index(idx)
    {}
    void paintEvent(QPaintEvent *event)
    {
        QPainter p(this);
        QPalette pal = palette();
        if (m_index % 2 == 0)
            p.fillRect(event->rect(), pal.base());
        else
            p.fillRect(event->rect(), pal.alternateBase());
    }

private:
    int m_index;
};

void AspectList::addToLayout(Layouting::LayoutItem &parent)
{
    using namespace Layouting;

    QScrollArea *scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setMaximumHeight(100);
    scrollArea->setMinimumHeight(100);
    scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto fill = [this, scrollArea]() mutable {
        if (scrollArea->widget())
            delete scrollArea->takeWidget();

        auto add = new QPushButton(Tr::tr("Add"));
        QObject::connect(add, &QPushButton::clicked, scrollArea, [this] {
            addItem(d->createItem());
        });

        Column column{noMargin()};

        forEachItem<BaseAspect>([&column, this](const std::shared_ptr<BaseAspect> &item, int idx) {
            auto removeBtn = new IconButton;
            removeBtn->setIcon(Utils::Icons::EDIT_CLEAR.icon());
            removeBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            QObject::connect(removeBtn, &QPushButton::clicked, removeBtn, [this, item] {
                removeItem(item);
            });
            ColoredRow *rowWdgt = new ColoredRow(idx);
            // clang-format off
            auto row = Row {
                *item,
                removeBtn,
                spacing(5),
            };
            // clang-format on
            row.attachTo(rowWdgt);
            column.addItem(rowWdgt);
        });

        ColoredRow *rowWdgt = new ColoredRow(size());
        Row{st, add}.attachTo(rowWdgt);
        column.addItem(rowWdgt);

        QWidget *contentWidget = column.emerge();
        contentWidget->layout()->setSpacing(1);

        scrollArea->setWidget(contentWidget);
    };

    fill();
    QObject::connect(this, &AspectList::volatileValueChanged, scrollArea, fill);

    parent.addItem(scrollArea);
}

StringSelectionAspect::StringSelectionAspect(AspectContainer *container)
    : TypedAspect<QString>(container)
{}

QStandardItem *StringSelectionAspect::itemById(const QString &id)
{
    for (int i = 0; i < m_model->rowCount(); ++i) {
        auto cur = m_model->item(i);
        if (cur->data() == id)
            return cur;
    }

    return nullptr;
}

void StringSelectionAspect::bufferToGui()
{
    if (!m_model) {
        m_undoable.setSilently(m_buffer);
        return;
    }

    auto selected = itemById(m_buffer);
    if (selected) {
        m_undoable.setSilently(selected->data().toString());
        m_selectionModel->setCurrentIndex(selected->index(),
                                          QItemSelectionModel::SelectionFlag::ClearAndSelect);
        return;
    }

    if (m_model->rowCount() > 0) {
        m_undoable.setSilently(m_model->item(0)->data().toString());
        m_selectionModel->setCurrentIndex(m_model->item(0)->index(),
                                          QItemSelectionModel::SelectionFlag::ClearAndSelect);
    } else {
        m_undoable.setSilently(m_buffer);
        m_selectionModel->setCurrentIndex(QModelIndex(), QItemSelectionModel::SelectionFlag::Clear);
    }

    handleGuiChanged();
}

bool StringSelectionAspect::guiToBuffer()
{
    if (!m_model)
        return false;

    auto oldBuffer = m_buffer;

    m_buffer = m_undoable.get();

    return oldBuffer != m_buffer;
}

void StringSelectionAspect::addToLayout(Layouting::LayoutItem &parent)
{
    QTC_ASSERT(m_fillCallback, return);

    auto cb = [this](const QList<QStandardItem *> &items) {
        m_model->clear();
        for (QStandardItem *item : items)
            m_model->appendRow(item);

        bufferToGui();
    };

    if (!m_model) {
        m_model = new QStandardItemModel(this);
        m_selectionModel = new QItemSelectionModel(m_model);

        connect(this, &StringSelectionAspect::refillRequested, this, [this, cb] {
            m_fillCallback(cb);
        });

        m_fillCallback(cb);
    }

    QComboBox *comboBox = new QComboBox();
    comboBox->setInsertPolicy(QComboBox::InsertPolicy::NoInsert);
    comboBox->setEditable(true);
    comboBox->completer()->setCompletionMode(QCompleter::PopupCompletion);
    comboBox->completer()->setFilterMode(Qt::MatchContains);

    comboBox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    comboBox->setCurrentText(value());
    comboBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

    comboBox->setModel(m_model);

    connect(m_selectionModel,
            &QItemSelectionModel::currentChanged,
            comboBox,
            [comboBox](QModelIndex currentIdx) {
                if (currentIdx.isValid() && comboBox->currentIndex() != currentIdx.row())
                    comboBox->setCurrentIndex(currentIdx.row());
            });

    connect(comboBox, &QComboBox::activated, this, [this](int idx) {
        QModelIndex modelIdx = m_model->index(idx, 0);
        if (!modelIdx.isValid())
            return;

        QString newValue = m_model->index(idx, 0).data(Qt::UserRole + 1).toString();
        if (newValue.isEmpty())
            return;

        m_undoable.set(undoStack(), newValue);
        bufferToGui();
    });

    connect(&m_undoable.m_signal, &UndoSignaller::changed, comboBox, [this, comboBox]() {
        auto item = itemById(m_undoable.get());
        if (item)
            m_selectionModel->setCurrentIndex(item->index(), QItemSelectionModel::ClearAndSelect);
        else
            comboBox->setCurrentText(m_undoable.get());

        handleGuiChanged();
    });

    if (m_selectionModel->currentIndex().isValid())
        comboBox->setCurrentIndex(m_selectionModel->currentIndex().row());

    return addLabeledItem(parent, comboBox);
}

} // namespace Utils
