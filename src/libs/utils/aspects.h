// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "filepath.h"
#include "id.h"
#include "infolabel.h"
#include "macroexpander.h"
#include "pathchooser.h"
#include "qtcsettings.h"
#include "store.h"

#include <functional>
#include <memory>
#include <optional>

#include <QUndoCommand>

QT_BEGIN_NAMESPACE
class QAction;
class QSettings;
class QUndoStack;
class QStandardItem;
class QStandardItemModel;
class QItemSelectionModel;
QT_END_NAMESPACE

namespace Layouting {
class LayoutItem;
}

namespace Utils {

class AspectContainer;
class BoolAspect;
class CheckableDecider;

namespace Internal {
class AspectContainerPrivate;
class BaseAspectPrivate;
class BoolAspectPrivate;
class ColorAspectPrivate;
class DoubleAspectPrivate;
class FilePathAspectPrivate;
class FilePathListAspectPrivate;
class IntegerAspectPrivate;
class MultiSelectionAspectPrivate;
class SelectionAspectPrivate;
class StringAspectPrivate;
class StringListAspectPrivate;
class TextDisplayPrivate;
class CheckableAspectImplementation;
class AspectListPrivate;
} // Internal

class QTCREATOR_UTILS_EXPORT BaseAspect : public QObject
{
    Q_OBJECT

public:
    BaseAspect(AspectContainer *container = nullptr);
    ~BaseAspect() override;

    Id id() const;
    void setId(Id id);

    enum Announcement { DoEmit, BeQuiet };

    virtual QVariant volatileVariantValue() const;
    virtual QVariant variantValue() const;
    virtual void setVariantValue(const QVariant &value, Announcement = DoEmit);

    virtual QVariant defaultVariantValue() const;
    virtual void setDefaultVariantValue(const QVariant &value);
    virtual bool isDefaultValue() const;

    Key settingsKey() const;
    void setSettingsKey(const Key &settingsKey);
    void setSettingsKey(const Key &group, const Key &key);

    QString displayName() const;
    void setDisplayName(const QString &displayName);

    QString toolTip() const;
    void setToolTip(const QString &tooltip);

    bool isVisible() const;
    void setVisible(bool visible);

    bool isAutoApply() const;
    virtual void setAutoApply(bool on);

    virtual void setUndoStack(QUndoStack *undoStack);
    QUndoStack *undoStack() const;

    bool isEnabled() const;
    void setEnabled(bool enabled);
    void setEnabler(BoolAspect *checker);

    bool isReadOnly() const;
    void setReadOnly(bool enabled);

    void setSpan(int x, int y = 1);

    QString labelText() const;
    void setLabelText(const QString &labelText);
    void setLabelPixmap(const QPixmap &labelPixmap);
    void setIcon(const QIcon &labelIcon);

    using ConfigWidgetCreator = std::function<QWidget *()>;
    void setConfigWidgetCreator(const ConfigWidgetCreator &configWidgetCreator);
    QWidget *createConfigWidget() const;

    virtual QAction *action();

    AspectContainer *container() const;

    virtual void fromMap(const Store &map);
    virtual void toMap(Store &map) const;
    virtual void toActiveMap(Store &map) const { toMap(map); }
    virtual void volatileToMap(Store &map) const;

    virtual void addToLayout(Layouting::LayoutItem &parent);

    virtual void readSettings();
    virtual void writeSettings() const;

    using SavedValueTransformation = std::function<QVariant(const QVariant &)>;
    void setFromSettingsTransformation(const SavedValueTransformation &transform);
    void setToSettingsTransformation(const SavedValueTransformation &transform);
    QVariant toSettingsValue(const QVariant &val) const;
    QVariant fromSettingsValue(const QVariant &val) const;

    virtual void apply();
    virtual void cancel();
    virtual void finish();
    virtual bool isDirty();
    bool hasAction() const;

    struct QTCREATOR_UTILS_EXPORT Changes
    {
        Changes();

        unsigned internalFromOutside : 1;
        unsigned internalFromBuffer : 1;
        unsigned bufferFromOutside : 1;
        unsigned bufferFromInternal : 1;
        unsigned bufferFromGui : 1;
    };

    void announceChanges(Changes changes, Announcement howToAnnounce = DoEmit);

    class QTCREATOR_UTILS_EXPORT Data
    {
    public:
        // The (unique) address of the "owning" aspect's meta object is used as identifier.
        using ClassId = const void *;

        virtual ~Data();

        Id id() const { return m_id; }
        ClassId classId() const { return m_classId; }
        Data *clone() const { return m_cloner(this); }

        QVariant value;

        class Ptr {
        public:
            Ptr() = default;
            explicit Ptr(const Data *data) : m_data(data) {}
            Ptr(const Ptr &other) { m_data = other.m_data->clone(); }
            ~Ptr() { delete m_data; }

            void operator=(const Ptr &other);
            void assign(const Data *other) { delete m_data; m_data = other; }

            const Data *get() const { return m_data; }

        private:
            const Data *m_data = nullptr;
        };

    protected:
        friend class BaseAspect;
        Id m_id;
        ClassId m_classId = 0;
        std::function<Data *(const Data *)> m_cloner;
    };

    using DataCreator = std::function<Data *()>;
    using DataCloner = std::function<Data *(const Data *)>;
    using DataExtractor = std::function<void(Data *data)>;

    Data::Ptr extractData() const;

    static void setQtcSettings(QtcSettings *settings);
    static QtcSettings *qtcSettings();

    // This is expensive. Do not use without good reason
    void writeToSettingsImmediatly() const;

signals:
    void changed(); // "internal"
    void volatileValueChanged();
    void labelLinkActivated(const QString &link);
    void checkedChanged();
    void enabledChanged();
    void labelTextChanged();
    void labelPixmapChanged();

protected:
    virtual bool internalToBuffer();
    virtual bool bufferToInternal();
    virtual void bufferToGui();
    virtual bool guiToBuffer();

    virtual void handleGuiChanged();

    QLabel *createLabel();
    void addLabeledItem(Layouting::LayoutItem &parent, QWidget *widget);

    void setDataCreatorHelper(const DataCreator &creator) const;
    void setDataClonerHelper(const DataCloner &cloner) const;
    void addDataExtractorHelper(const DataExtractor &extractor) const;

    template <typename AspectClass, typename DataClass, typename Type>
    void addDataExtractor(AspectClass *aspect,
                          Type(AspectClass::*p)() const,
                          Type DataClass::*q) {
        setDataCreatorHelper([] {
            return new DataClass;
        });
        setDataClonerHelper([](const Data *data) {
            return new DataClass(*static_cast<const DataClass *>(data));
        });
        addDataExtractorHelper([aspect, p, q](Data *data) {
            static_cast<DataClass *>(data)->*q = (aspect->*p)();
        });
    }

    template <class Widget, typename ...Args>
    Widget *createSubWidget(Args && ...args) {
        auto w = new Widget(args...);
        registerSubWidget(w);
        return w;
    }

    void registerSubWidget(QWidget *widget);
    static void saveToMap(Store &data, const QVariant &value,
                          const QVariant &defaultValue, const Key &key);

    void forEachSubWidget(const std::function<void(QWidget *)> &func);

protected:
    template <class Value>
    static bool updateStorage(Value &target, const Value &val)
    {
        if (target == val)
            return false;
        target = val;
        return true;
    }

private:
    std::unique_ptr<Internal::BaseAspectPrivate> d;
    friend class Internal::CheckableAspectImplementation;
};

QTCREATOR_UTILS_EXPORT void createItem(Layouting::LayoutItem *item, const BaseAspect &aspect);
QTCREATOR_UTILS_EXPORT void createItem(Layouting::LayoutItem *item, const BaseAspect *aspect);

template <typename ValueType>
class TypedAspect : public BaseAspect
{
public:
    TypedAspect(AspectContainer *container = nullptr)
        : BaseAspect(container)
    {
        addDataExtractor(this, &TypedAspect::value, &Data::value);
    }

    struct Data : BaseAspect::Data
    {
        ValueType value;
    };

    ValueType operator()() const { return m_internal; }
    ValueType value() const { return m_internal; }
    ValueType defaultValue() const { return m_default; }
    ValueType volatileValue() const { return m_buffer; }

    // We assume that this is only used in the ctor and no signalling is needed.
    // If it is used elsewhere changes have to be detected and signalled externally.
    void setDefaultValue(const ValueType &value)
    {
        m_default = value;
        m_internal = value;
        if (internalToBuffer()) // Might be more than a plain copy.
            bufferToGui();
    }

    bool isDefaultValue() const override
    {
        return m_default == m_internal;
    }

    void setValue(const ValueType &value, Announcement howToAnnounce = DoEmit)
    {
        Changes changes;
        changes.internalFromOutside = updateStorage(m_internal, value);
        if (internalToBuffer()) {
            changes.bufferFromInternal = true;
            bufferToGui();
        }
        announceChanges(changes, howToAnnounce);
    }

    void setVolatileValue(const ValueType &value, Announcement howToAnnounce = DoEmit)
    {
        Changes changes;
        if (updateStorage(m_buffer, value)) {
            changes.bufferFromOutside = true;
            bufferToGui();
        }
        if (isAutoApply() && bufferToInternal())
            changes.internalFromBuffer = true;
        announceChanges(changes, howToAnnounce);
    }

protected:
    bool isDirty() override
    {
        return m_internal != m_buffer;
    }

    bool internalToBuffer() override
    {
        return updateStorage(m_buffer, m_internal);
    }

    bool bufferToInternal() override
    {
        return updateStorage(m_internal, m_buffer);
    }

    QVariant variantValue() const override
    {
        return QVariant::fromValue<ValueType>(m_internal);
    }

    QVariant volatileVariantValue() const override
    {
        return QVariant::fromValue<ValueType>(m_buffer);
    }

    void setVariantValue(const QVariant &value, Announcement howToAnnounce = DoEmit) override
    {
        setValue(value.value<ValueType>(), howToAnnounce);
    }

    QVariant defaultVariantValue() const override
    {
        return QVariant::fromValue<ValueType>(m_default);
    }

    void setDefaultVariantValue(const QVariant &value) override
    {
        setDefaultValue(value.value<ValueType>());
    }

    ValueType m_default{};
    ValueType m_internal{};
    ValueType m_buffer{};
};

template <typename ValueType>
class FlexibleTypedAspect : public TypedAspect<ValueType>
{
public:
    using Base = TypedAspect<ValueType>;
    using Updater = std::function<bool(ValueType &, const ValueType &)>;

    using Base::Base;

    void setInternalToBuffer(const Updater &updater) { m_internalToBuffer = updater; }
    void setBufferToInternal(const Updater &updater) { m_bufferToInternal = updater; }
    void setInternalToExternal(const Updater &updater) { m_internalToExternal = updater; }

protected:
    bool internalToBuffer() override
    {
        if (m_internalToBuffer)
            return m_internalToBuffer(Base::m_buffer, Base::m_internal);
        return Base::internalToBuffer();
    }

    bool bufferToInternal() override
    {
        if (m_bufferToInternal)
            return m_bufferToInternal(Base::m_internal, Base::m_buffer);
        return Base::bufferToInternal();
    }

    ValueType expandedValue()
    {
        if (!m_internalToExternal)
            return Base::m_internal;
        ValueType val;
        m_internalToExternal(val, Base::m_internal);
        return val;
    }

    Updater m_internalToBuffer;
    Updater m_bufferToInternal;
    Updater m_internalToExternal;
};

class QTCREATOR_UTILS_EXPORT BoolAspect : public TypedAspect<bool>
{
    Q_OBJECT

public:
    BoolAspect(AspectContainer *container = nullptr);
    ~BoolAspect() override;

    void addToLayout(Layouting::LayoutItem &parent) override;
    std::function<void(QObject *)> groupChecker();

    Utils::CheckableDecider askAgainCheckableDecider();
    Utils::CheckableDecider doNotAskAgainCheckableDecider();

    QAction *action() override;

    enum class LabelPlacement { AtCheckBox, Compact, InExtraLabel };
    void setLabel(const QString &labelText,
                  LabelPlacement labelPlacement = LabelPlacement::InExtraLabel);
    void setLabelPlacement(LabelPlacement labelPlacement);

    Layouting::LayoutItem adoptButton(QAbstractButton *button);

private:
    void addToLayoutHelper(Layouting::LayoutItem &parent, QAbstractButton *button);

    void bufferToGui() override;
    bool guiToBuffer() override;

    std::unique_ptr<Internal::BoolAspectPrivate> d;
};

class QTCREATOR_UTILS_EXPORT ColorAspect : public TypedAspect<QColor>
{
    Q_OBJECT

public:
    ColorAspect(AspectContainer *container = nullptr);
    ~ColorAspect() override;

    void addToLayout(Layouting::LayoutItem &parent) override;

private:
    void bufferToGui() override;
    bool guiToBuffer() override;

    std::unique_ptr<Internal::ColorAspectPrivate> d;
};

class QTCREATOR_UTILS_EXPORT SelectionAspect : public TypedAspect<int>
{
    Q_OBJECT

public:
    SelectionAspect(AspectContainer *container = nullptr);
    ~SelectionAspect() override;

    void addToLayout(Layouting::LayoutItem &parent) override;
    void finish() override;

    QString stringValue() const;
    void setStringValue(const QString &val);

    void setDefaultValue(const QString &val);
    void setDefaultValue(int val);

    QVariant itemValue() const;

    enum class DisplayStyle { RadioButtons, ComboBox };
    void setDisplayStyle(DisplayStyle style);

    class Option
    {
    public:
        Option(const QString &displayName, const QString &toolTip, const QVariant &itemData)
            : displayName(displayName), tooltip(toolTip), itemData(itemData)
        {}
        QString displayName;
        QString tooltip;
        QVariant itemData;
        bool enabled = true;
    };

    void addOption(const QString &displayName, const QString &toolTip = {});
    void addOption(const Option &option);
    int indexForDisplay(const QString &displayName) const;
    QString displayForIndex(int index) const;
    int indexForItemValue(const QVariant &value) const;
    QVariant itemValueForIndex(int index) const;

protected:
    void bufferToGui() override;
    bool guiToBuffer() override;

private:
    std::unique_ptr<Internal::SelectionAspectPrivate> d;
};

class QTCREATOR_UTILS_EXPORT MultiSelectionAspect : public TypedAspect<QStringList>
{
    Q_OBJECT

public:
    MultiSelectionAspect(AspectContainer *container = nullptr);
    ~MultiSelectionAspect() override;

    void addToLayout(Layouting::LayoutItem &parent) override;

    enum class DisplayStyle { ListView };
    void setDisplayStyle(DisplayStyle style);

    QStringList allValues() const;
    void setAllValues(const QStringList &val);

protected:
    void bufferToGui() override;
    bool guiToBuffer() override;

private:
    std::unique_ptr<Internal::MultiSelectionAspectPrivate> d;
};

enum class UncheckedSemantics { Disabled, ReadOnly };
enum class CheckBoxPlacement { Top, Right };

class QTCREATOR_UTILS_EXPORT StringAspect : public TypedAspect<QString>
{
    Q_OBJECT

public:
    StringAspect(AspectContainer *container = nullptr);
    ~StringAspect() override;

    void addToLayout(Layouting::LayoutItem &parent) override;

    QString operator()() const { return expandedValue(); }
    QString expandedValue() const;

    // Hook between UI and StringAspect:
    using ValueAcceptor = std::function<std::optional<QString>(const QString &, const QString &)>;
    void setValueAcceptor(ValueAcceptor &&acceptor);

    void setShowToolTipOnLabel(bool show);
    void setDisplayFilter(const std::function<QString (const QString &)> &displayFilter);
    void setPlaceHolderText(const QString &placeHolderText);
    void setHistoryCompleter(const Key &historyCompleterKey);
    void setAcceptRichText(bool acceptRichText);
    void setMacroExpanderProvider(const MacroExpanderProvider &expanderProvider);
    void setUseGlobalMacroExpander();
    void setUseResetButton();
    void setValidationFunction(const FancyLineEdit::ValidationFunction &validator);
    void setAutoApplyOnEditingFinished(bool applyOnEditingFinished);
    void setElideMode(Qt::TextElideMode elideMode);

    void makeCheckable(CheckBoxPlacement checkBoxPlacement, const QString &optionalLabel, const Key &optionalBaseKey);
    bool isChecked() const;
    void setChecked(bool checked);

    enum DisplayStyle {
        LabelDisplay,
        LineEditDisplay,
        TextEditDisplay,
        PasswordLineEditDisplay,
    };
    void setDisplayStyle(DisplayStyle style);

    void fromMap(const Utils::Store &map) override;
    void toMap(Utils::Store &map) const override;
    void volatileToMap(Utils::Store &map) const override;

signals:
    void validChanged(bool validState);
    void elideModeChanged(Qt::TextElideMode elideMode);
    void historyCompleterKeyChanged(const Key &historyCompleterKey);
    void acceptRichTextChanged(bool acceptRichText);
    void validationFunctionChanged(const FancyLineEdit::ValidationFunction &validator);
    void placeholderTextChanged(const QString &placeholderText);

protected:
    void bufferToGui() override;
    bool guiToBuffer() override;

    bool internalToBuffer() override;
    bool bufferToInternal() override;

    std::unique_ptr<Internal::StringAspectPrivate> d;
};

class QTCREATOR_UTILS_EXPORT FilePathAspect : public TypedAspect<QString>
{
    Q_OBJECT

public:
    FilePathAspect(AspectContainer *container = nullptr);
    ~FilePathAspect();

    struct Data : BaseAspect::Data
    {
        QString value;
        FilePath filePath;
    };

    FilePath operator()() const;
    FilePath expandedValue() const;
    QString value() const;
    void setValue(const FilePath &filePath, Announcement howToAnnounce = DoEmit);
    void setValue(const QString &filePath, Announcement howToAnnounce = DoEmit);
    void setDefaultValue(const QString &filePath);

    void setPromptDialogFilter(const QString &filter);
    void setPromptDialogTitle(const QString &title);
    void setCommandVersionArguments(const QStringList &arguments);
    void setAllowPathFromDevice(bool allowPathFromDevice);
    void setValidatePlaceHolder(bool validatePlaceHolder);
    void setOpenTerminalHandler(const std::function<void()> &openTerminal);
    void setExpectedKind(const PathChooser::Kind expectedKind);
    void setEnvironment(const Environment &env);
    void setBaseFileName(const FilePath &baseFileName);

    void setPlaceHolderText(const QString &placeHolderText);
    void setValidationFunction(const FancyLineEdit::ValidationFunction &validator);
    void setDisplayFilter(const std::function<QString (const QString &)> &displayFilter);
    void setHistoryCompleter(const Key &historyCompleterKey);
    void setMacroExpanderProvider(const MacroExpanderProvider &expanderProvider);
    void setShowToolTipOnLabel(bool show);
    void setAutoApplyOnEditingFinished(bool applyOnEditingFinished);

    void validateInput();

    void makeCheckable(CheckBoxPlacement checkBoxPlacement, const QString &optionalLabel, const Key &optionalBaseKey);
    bool isChecked() const;
    void setChecked(bool checked);

    // Hook between UI and StringAspect:
    using ValueAcceptor = std::function<std::optional<QString>(const QString &, const QString &)>;
    void setValueAcceptor(ValueAcceptor &&acceptor);

    PathChooser *pathChooser() const; // Avoid to use.

    void addToLayout(Layouting::LayoutItem &parent) override;

    void fromMap(const Utils::Store &map) override;
    void toMap(Utils::Store &map) const override;
    void volatileToMap(Utils::Store &map) const override;

signals:
    void validChanged(bool validState);

protected:
    void bufferToGui() override;
    bool guiToBuffer() override;

    bool internalToBuffer() override;
    bool bufferToInternal() override;

    std::unique_ptr<Internal::FilePathAspectPrivate> d;
};

class QTCREATOR_UTILS_EXPORT IntegerAspect : public TypedAspect<qint64>
{
    Q_OBJECT

public:
    IntegerAspect(AspectContainer *container = nullptr);
    ~IntegerAspect() override;

    void addToLayout(Layouting::LayoutItem &parent) override;

    void setRange(qint64 min, qint64 max);
    void setLabel(const QString &label); // FIXME: Use setLabelText
    void setPrefix(const QString &prefix);
    void setSuffix(const QString &suffix);
    void setDisplayIntegerBase(int base);
    void setDisplayScaleFactor(qint64 factor);
    void setSpecialValueText(const QString &specialText);
    void setSingleStep(qint64 step);

    struct Data : BaseAspect::Data { qint64 value = 0; };

protected:
    void bufferToGui() override;
    bool guiToBuffer() override;

private:
    std::unique_ptr<Internal::IntegerAspectPrivate> d;
};

class QTCREATOR_UTILS_EXPORT DoubleAspect : public TypedAspect<double>
{
    Q_OBJECT

public:
    DoubleAspect(AspectContainer *container = nullptr);
    ~DoubleAspect() override;

    void addToLayout(Layouting::LayoutItem &parent) override;

    void setRange(double min, double max);
    void setPrefix(const QString &prefix);
    void setSuffix(const QString &suffix);
    void setSpecialValueText(const QString &specialText);
    void setSingleStep(double step);

protected:
    void bufferToGui() override;
    bool guiToBuffer() override;

private:
    std::unique_ptr<Internal::DoubleAspectPrivate> d;
};

class QTCREATOR_UTILS_EXPORT TriState
{
    enum Value { EnabledValue, DisabledValue, DefaultValue };
    explicit TriState(Value v) : m_value(v) {}

public:
    TriState() = default;

    int toInt() const { return int(m_value); }
    QVariant toVariant() const { return int(m_value); }
    static TriState fromInt(int value);
    static TriState fromVariant(const QVariant &variant);

    static const TriState Enabled;
    static const TriState Disabled;
    static const TriState Default;

    friend bool operator==(TriState a, TriState b) { return a.m_value == b.m_value; }
    friend bool operator!=(TriState a, TriState b) { return a.m_value != b.m_value; }

private:
    Value m_value = DefaultValue;
};

class QTCREATOR_UTILS_EXPORT TriStateAspect : public SelectionAspect
{
    Q_OBJECT

public:
    TriStateAspect(AspectContainer *container = nullptr,
                   const QString &onString = {},
                   const QString &offString = {},
                   const QString &defaultString = {});

    TriState operator()() const { return value(); }
    TriState value() const;
    void setValue(TriState setting);

    TriState defaultValue() const;
    void setDefaultValue(TriState setting);
};

class QTCREATOR_UTILS_EXPORT StringListAspect : public TypedAspect<QStringList>
{
    Q_OBJECT

public:
    StringListAspect(AspectContainer *container = nullptr);
    ~StringListAspect() override;

    void addToLayout(Layouting::LayoutItem &parent) override;

    void appendValue(const QString &value, bool allowDuplicates = true);
    void removeValue(const QString &value);
    void appendValues(const QStringList &values, bool allowDuplicates = true);
    void removeValues(const QStringList &values);

private:
    std::unique_ptr<Internal::StringListAspectPrivate> d;
};

class QTCREATOR_UTILS_EXPORT FilePathListAspect : public TypedAspect<QStringList>
{
    Q_OBJECT

public:
    FilePathListAspect(AspectContainer *container = nullptr);
    ~FilePathListAspect() override;

    FilePaths operator()() const;

    bool guiToBuffer() override;
    void bufferToGui() override;

    void addToLayout(Layouting::LayoutItem &parent) override;
    void setPlaceHolderText(const QString &placeHolderText);

    void appendValue(const FilePath &path, bool allowDuplicates = true);
    void removeValue(const FilePath &path);
    void appendValues(const FilePaths &values, bool allowDuplicates = true);
    void removeValues(const FilePaths &values);

private:
    std::unique_ptr<Internal::FilePathListAspectPrivate> d;
};

class QTCREATOR_UTILS_EXPORT IntegersAspect : public TypedAspect<QList<int>>
{
    Q_OBJECT

public:
    IntegersAspect(AspectContainer *container = nullptr);
    ~IntegersAspect() override;

    void addToLayout(Layouting::LayoutItem &parent) override;
};

class QTCREATOR_UTILS_EXPORT TextDisplay : public BaseAspect
{
    Q_OBJECT

public:
    explicit TextDisplay(AspectContainer *container,
                         const QString &message = {},
                         InfoLabel::InfoType type = InfoLabel::None);
    ~TextDisplay() override;

    void addToLayout(Layouting::LayoutItem &parent) override;

    void setIconType(InfoLabel::InfoType t);
    void setText(const QString &message);

private:
    std::unique_ptr<Internal::TextDisplayPrivate> d;
};

class QTCREATOR_UTILS_EXPORT AspectContainerData
{
public:
    AspectContainerData() = default;

    const BaseAspect::Data *aspect(Id instanceId) const;
    const BaseAspect::Data *aspect(BaseAspect::Data::ClassId classId) const;

    void append(const BaseAspect::Data::Ptr &data);

    template <typename T> const typename T::Data *aspect() const
    {
        return static_cast<const typename T::Data *>(aspect(&T::staticMetaObject));
    }

private:
    QList<BaseAspect::Data::Ptr> m_data; // Owned.
};

class QTCREATOR_UTILS_EXPORT SettingsGroupNester
{
    Q_DISABLE_COPY_MOVE(SettingsGroupNester)

public:
    explicit SettingsGroupNester(const QStringList &groups);
    ~SettingsGroupNester();

private:
    const int m_groupCount;
};

class QTCREATOR_UTILS_EXPORT AspectContainer : public BaseAspect
{
    Q_OBJECT

public:
    AspectContainer();
    ~AspectContainer();

    AspectContainer(const AspectContainer &) = delete;
    AspectContainer &operator=(const AspectContainer &) = delete;

    void registerAspect(BaseAspect *aspect, bool takeOwnership = false);
    void registerAspects(const AspectContainer &aspects);

    void fromMap(const Utils::Store &map) override;
    void toMap(Utils::Store &map) const override;
    void volatileToMap(Utils::Store &map) const override;

    void readSettings() override;
    void writeSettings() const override;

    void setSettingsGroup(const QString &groupKey);
    void setSettingsGroups(const QString &groupKey, const QString &subGroupKey);
    QStringList settingsGroups() const;

    void apply() override;
    void cancel() override;
    void finish() override;

    void reset();
    bool equals(const AspectContainer &other) const;
    void copyFrom(const AspectContainer &other);
    void setAutoApply(bool on) override;
    bool isDirty() override;
    void setUndoStack(QUndoStack *undoStack) override;

    template <typename T> T *aspect() const
    {
        for (BaseAspect *aspect : aspects())
            if (T *result = qobject_cast<T *>(aspect))
                return result;
        return nullptr;
    }

    BaseAspect *aspect(Id id) const;

    template <typename T> T *aspect(Id id) const
    {
        return qobject_cast<T*>(aspect(id));
    }

    void forEachAspect(const std::function<void(BaseAspect *)> &run) const;

    const QList<BaseAspect *> &aspects() const;

    using const_iterator = QList<BaseAspect *>::const_iterator;
    using value_type = QList<BaseAspect *>::value_type;

    const_iterator begin() const;
    const_iterator end() const;

    void setLayouter(const std::function<Layouting::LayoutItem()> &layouter);
    std::function<Layouting::LayoutItem()> layouter() const;

signals:
    void applied();
    void changed();
    void fromMapFinished();

private:
    std::unique_ptr<Internal::AspectContainerPrivate> d;
};

// Because QObject cannot be a template
class QTCREATOR_UTILS_EXPORT UndoSignaller : public QObject
{
    Q_OBJECT
public:
    void emitChanged() { emit changed(); }
signals:
    void changed();
};

template<class T>
class QTCREATOR_UTILS_EXPORT UndoableValue
{
public:
    class UndoCmd : public QUndoCommand
    {
    public:
        UndoCmd(UndoableValue<T> *value, const T &oldValue, const T &newValue)
            : m_value(value)
            , m_oldValue(oldValue)
            , m_newValue(newValue)
        {}

        void undo() override { m_value->setInternal(m_oldValue); }
        void redo() override { m_value->setInternal(m_newValue); }

    private:
        UndoableValue<T> *m_value;
        T m_oldValue;
        T m_newValue;
    };

    void set(QUndoStack *stack, const T &value)
    {
        if (m_value == value)
            return;

        if (stack)
            stack->push(new UndoCmd(this, m_value, value));
        else
            setInternal(value);
    }

    void setSilently(const T &value) { m_value = value; }
    void setWithoutUndo(const T &value) { setInternal(value); }

    T get() const { return m_value; }

    UndoSignaller m_signal;

private:
    void setInternal(const T &value)
    {
        m_value = value;
        m_signal.emitChanged();
    }

private:
    T m_value;
};

class QTCREATOR_UTILS_EXPORT AspectList : public Utils::BaseAspect
{
public:
    using CreateItem = std::function<std::shared_ptr<BaseAspect>()>;
    using ItemCallback = std::function<void(std::shared_ptr<BaseAspect>)>;

    AspectList(Utils::AspectContainer *container = nullptr);
    ~AspectList() override;

    void fromMap(const Utils::Store &map) override;
    void toMap(Utils::Store &map) const override;

    void volatileToMap(Utils::Store &map) const override;
    QVariantList toList(bool v) const;

    QList<std::shared_ptr<BaseAspect>> items() const;
    QList<std::shared_ptr<BaseAspect>> volatileItems() const;

    std::shared_ptr<BaseAspect> createAndAddItem();
    std::shared_ptr<BaseAspect> addItem(const std::shared_ptr<BaseAspect> &item);
    std::shared_ptr<BaseAspect> actualAddItem(const std::shared_ptr<BaseAspect> &item);

    void removeItem(const std::shared_ptr<BaseAspect> &item);
    void actualRemoveItem(const std::shared_ptr<BaseAspect> &item);
    void clear();

    void apply() override;

    void setCreateItemFunction(CreateItem createItem);

    template<class T>
    void forEachItem(std::function<void(const std::shared_ptr<T> &)> callback)
    {
        for (const auto &item : volatileItems())
            callback(std::static_pointer_cast<T>(item));
    }

    template<class T>
    void forEachItem(std::function<void(const std::shared_ptr<T> &, int)> callback)
    {
        int idx = 0;
        for (const auto &item : volatileItems())
            callback(std::static_pointer_cast<T>(item), idx++);
    }

    void setItemAddedCallback(const ItemCallback &callback);
    void setItemRemovedCallback(const ItemCallback &callback);

    template<class T>
    void setItemAddedCallback(const std::function<void(const std::shared_ptr<T>)> &callback)
    {
        setItemAddedCallback([callback](const std::shared_ptr<BaseAspect> &item) {
            callback(std::static_pointer_cast<T>(item));
        });
    }

    template<class T>
    void setItemRemovedCallback(const std::function<void(const std::shared_ptr<T>)> &callback)
    {
        setItemRemovedCallback([callback](const std::shared_ptr<BaseAspect> &item) {
            callback(std::static_pointer_cast<T>(item));
        });
    }

    qsizetype size() const;
    bool isDirty() override;

    QVariant volatileVariantValue() const override { return {}; }

    void addToLayout(Layouting::LayoutItem &parent) override;

private:
    std::unique_ptr<Internal::AspectListPrivate> d;
};

class QTCREATOR_UTILS_EXPORT StringSelectionAspect : public Utils::TypedAspect<QString>
{
    Q_OBJECT
public:
    StringSelectionAspect(Utils::AspectContainer *container = nullptr);

    void addToLayout(Layouting::LayoutItem &parent) override;

    using ResultCallback = std::function<void(QList<QStandardItem *> items)>;
    using FillCallback = std::function<void(ResultCallback)>;
    void setFillCallback(FillCallback callback) { m_fillCallback = callback; }

    void refill() { emit refillRequested(); }

    void bufferToGui() override;
    bool guiToBuffer() override;

signals:
    void refillRequested();

private:
    QStandardItem *itemById(const QString &id);

    FillCallback m_fillCallback;
    QStandardItemModel *m_model{nullptr};
    QItemSelectionModel *m_selectionModel{nullptr};

    Utils::UndoableValue<QString> m_undoable;
};

} // namespace Utils
