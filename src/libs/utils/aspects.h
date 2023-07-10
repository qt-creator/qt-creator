// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "filepath.h"
#include "id.h"
#include "infolabel.h"
#include "macroexpander.h"
#include "pathchooser.h"

#include <functional>
#include <memory>
#include <optional>

QT_BEGIN_NAMESPACE
class QAction;
class QSettings;
QT_END_NAMESPACE

namespace Layouting { class LayoutItem; }

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
class IntegerAspectPrivate;
class MultiSelectionAspectPrivate;
class SelectionAspectPrivate;
class StringAspectPrivate;
class StringListAspectPrivate;
class TextDisplayPrivate;
} // Internal

class QTCREATOR_UTILS_EXPORT BaseAspect : public QObject
{
    Q_OBJECT

public:
    BaseAspect(AspectContainer *container = nullptr);
    ~BaseAspect() override;

    Id id() const;
    void setId(Id id);

    virtual QVariant variantValue() const;
    virtual void setVariantValue(const QVariant &value);
    virtual void setVariantValueQuietly(const QVariant &value);

    virtual QVariant defaultVariantValue() const;
    virtual void setDefaultVariantValue(const QVariant &value);

    QString settingsKey() const;
    void setSettingsKey(const QString &settingsKey);
    void setSettingsKey(const QString &group, const QString &key);

    QString displayName() const;
    void setDisplayName(const QString &displayName);

    QString toolTip() const;
    void setToolTip(const QString &tooltip);

    bool isVisible() const;
    void setVisible(bool visible);

    bool isAutoApply() const;
    void setAutoApply(bool on);

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

    virtual void fromMap(const QVariantMap &map);
    virtual void toMap(QVariantMap &map) const;
    virtual void toActiveMap(QVariantMap &map) const { toMap(map); }

    virtual void addToLayout(Layouting::LayoutItem &parent);

    virtual void readSettings();
    virtual void writeSettings() const;

    using SavedValueTransformation = std::function<QVariant(const QVariant &)>;
    void setFromSettingsTransformation(const SavedValueTransformation &transform);
    void setToSettingsTransformation(const SavedValueTransformation &transform);
    QVariant toSettingsValue(const QVariant &val) const;
    QVariant fromSettingsValue(const QVariant &val) const;

    virtual void apply();
    virtual bool silentApply();
    virtual void cancel();
    virtual void finish();
    virtual bool isDirty();
    bool hasAction() const;

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

    static void setGlobalSettings(QSettings *settings);
    static QSettings *settings();

signals:
    void changed();
    void volatileValueChanged();
    void labelLinkActivated(const QString &link);

protected:
    virtual void internalToGui();
    virtual void guiToInternal();
    virtual bool internalToExternal();
    virtual void externalToInternal();

    QLabel *label() const;
    void setupLabel();
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
    static void saveToMap(QVariantMap &data, const QVariant &value,
                          const QVariant &defaultValue, const QString &key);

private:
    std::unique_ptr<Internal::BaseAspectPrivate> d;
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

    ValueType operator()() const { return m_external; }
    ValueType value() const { return m_external; }
    ValueType defaultValue() const { return m_default; }

    void setDefaultValue(const ValueType &value)
    {
        m_default = value;
        setValue(value);
    }

    void setValue(const ValueType &value)
    {
        m_external = value;
        const bool emitSignal = m_external != m_internal;
        externalToInternal();
        internalToGui();
        if (emitSignal)
            emit changed();
    }

    void setValueQuietly(const ValueType &value)
    {
        m_external = value;
        externalToInternal();
        internalToGui();
    }

    void setVolatileValue(const ValueType &value)
    {
        m_internal = value;
        internalToGui();
    }

    ValueType volatileValue() const
    {
        const_cast<TypedAspect *>(this)->guiToInternal();
        return m_internal;
    }

protected:
    bool isDirty() override
    {
        guiToInternal();
        return m_internal != m_external;
    }

    bool internalToExternal() override
    {
        if (m_external == m_internal)
            return false;
        m_external = m_internal;
        return true;
    }

    void externalToInternal() override
    {
        m_internal = m_external;
    }

    QVariant variantValue() const override
    {
        return QVariant::fromValue<ValueType>(m_external);
    }

    void setVariantValue(const QVariant &value) override
    {
        setValue(value.value<ValueType>());
    }

    void setVariantValueQuietly(const QVariant &value) override
    {
        setValueQuietly(value.value<ValueType>());
    }

    QVariant defaultVariantValue() const override
    {
        return QVariant::fromValue<ValueType>(m_default);
    }

    void setDefaultVariantValue(const QVariant &value) override
    {
        m_default = value.value<ValueType>();
    }

    ValueType m_default{};
    ValueType m_external{};
    ValueType m_internal{};
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

    enum class LabelPlacement { AtCheckBox, AtCheckBoxWithoutDummyLabel, InExtraLabel };
    void setLabel(const QString &labelText,
                  LabelPlacement labelPlacement = LabelPlacement::InExtraLabel);
    void setLabelPlacement(LabelPlacement labelPlacement);

    void adoptButton(QAbstractButton *button);

private:
    void internalToGui() override;
    void guiToInternal() override;

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
    void internalToGui() override;
    void guiToInternal() override;

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
    void internalToGui() override;
    void guiToInternal() override;

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
    void internalToGui() override;
    void guiToInternal() override;

private:
    std::unique_ptr<Internal::MultiSelectionAspectPrivate> d;
};

class QTCREATOR_UTILS_EXPORT StringAspect : public TypedAspect<QString>
{
    Q_OBJECT

public:
    StringAspect(AspectContainer *container = nullptr);
    ~StringAspect() override;

    struct Data : BaseAspect::Data
    {
        QString value;
        FilePath filePath;
    };

    void addToLayout(Layouting::LayoutItem &parent) override;

    // Hook between UI and StringAspect:
    using ValueAcceptor = std::function<std::optional<QString>(const QString &, const QString &)>;
    void setValueAcceptor(ValueAcceptor &&acceptor);

    QString value() const;

    void setShowToolTipOnLabel(bool show);

    void setDisplayFilter(const std::function<QString (const QString &)> &displayFilter);
    void setPlaceHolderText(const QString &placeHolderText);
    void setHistoryCompleter(const QString &historyCompleterKey);
    void setExpectedKind(const PathChooser::Kind expectedKind);
    void setEnvironment(const Environment &env);
    void setBaseFileName(const FilePath &baseFileName);
    void setUndoRedoEnabled(bool readOnly);
    void setAcceptRichText(bool acceptRichText);
    void setMacroExpanderProvider(const MacroExpanderProvider &expanderProvider);
    void setUseGlobalMacroExpander();
    void setUseResetButton();
    void setValidationFunction(const FancyLineEdit::ValidationFunction &validator);
    void setAutoApplyOnEditingFinished(bool applyOnEditingFinished);
    void setElideMode(Qt::TextElideMode elideMode);

    void validateInput();

    enum class UncheckedSemantics { Disabled, ReadOnly };
    enum class CheckBoxPlacement { Top, Right };
    void setUncheckedSemantics(UncheckedSemantics semantics);
    bool isChecked() const;
    void setChecked(bool checked);
    void makeCheckable(CheckBoxPlacement checkBoxPlacement, const QString &optionalLabel,
                       const QString &optionalBaseKey);

    enum DisplayStyle {
        LabelDisplay,
        LineEditDisplay,
        TextEditDisplay,
        PathChooserDisplay
    };
    void setDisplayStyle(DisplayStyle style);

    void fromMap(const QVariantMap &map) override;
    void toMap(QVariantMap &map) const override;

    FilePath filePath() const;

    PathChooser *pathChooser() const; // Avoid to use.

signals:
    void checkedChanged();
    void validChanged(bool validState);

protected:
    void internalToGui() override;
    void guiToInternal() override;

    std::unique_ptr<Internal::StringAspectPrivate> d;
};

class QTCREATOR_UTILS_EXPORT FilePathAspect : public StringAspect
{
public:
    FilePathAspect(AspectContainer *container = nullptr);

    FilePath operator()() const;
    FilePath value() const;
    QString stringValue() const;
    void setValue(const FilePath &filePath);
    void setDefaultValue(const FilePath &filePath);

    void setPromptDialogFilter(const QString &filter);
    void setPromptDialogTitle(const QString &title);
    void setCommandVersionArguments(const QStringList &arguments);
    void setAllowPathFromDevice(bool allowPathFromDevice);
    void setValidatePlaceHolder(bool validatePlaceHolder);
    void setOpenTerminalHandler(const std::function<void()> &openTerminal);
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
    void internalToGui() override;
    void guiToInternal() override;

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
    void internalToGui() override;
    void guiToInternal() override;

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

class QTCREATOR_UTILS_EXPORT AspectContainer : public QObject
{
    Q_OBJECT

public:
    AspectContainer(QObject *parent = nullptr);
    ~AspectContainer();

    AspectContainer(const AspectContainer &) = delete;
    AspectContainer &operator=(const AspectContainer &) = delete;

    void registerAspect(BaseAspect *aspect, bool takeOwnership = false);
    void registerAspects(const AspectContainer &aspects);

    template <class Aspect, typename ...Args>
    Aspect *addAspect(Args && ...args)
    {
        auto aspect = new Aspect(args...);
        registerAspect(aspect, true);
        return aspect;
    }

    void fromMap(const QVariantMap &map);
    void toMap(QVariantMap &map) const;

    void readSettings();
    void writeSettings() const;

    void setSettingsGroup(const QString &groupKey);
    void setSettingsGroups(const QString &groupKey, const QString &subGroupKey);

    void apply();
    void cancel();
    void finish();

    void reset();
    bool equals(const AspectContainer &other) const;
    void copyFrom(const AspectContainer &other);
    void setAutoApply(bool on);
    bool isDirty() const;

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

} // namespace Utils
