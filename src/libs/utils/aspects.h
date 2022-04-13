/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#pragma once

#include "fileutils.h"
#include "id.h"
#include "infolabel.h"
#include "macroexpander.h"
#include "optional.h"
#include "pathchooser.h"

#include <functional>
#include <memory>

QT_BEGIN_NAMESPACE
class QAction;
class QGroupBox;
class QSettings;
QT_END_NAMESPACE

namespace Utils {

class AspectContainer;
class BoolAspect;
class LayoutBuilder;

namespace Internal {
class AspectContainerPrivate;
class BaseAspectPrivate;
class BoolAspectPrivate;
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
    BaseAspect();
    ~BaseAspect() override;

    Utils::Id id() const;
    void setId(Utils::Id id);

    QVariant value() const;
    void setValue(const QVariant &value);
    bool setValueQuietly(const QVariant &value);

    QVariant defaultValue() const;
    void setDefaultValue(const QVariant &value);

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

    virtual void fromMap(const QVariantMap &map);
    virtual void toMap(QVariantMap &map) const;
    virtual void toActiveMap(QVariantMap &map) const { toMap(map); }

    virtual void addToLayout(LayoutBuilder &builder);

    virtual QVariant volatileValue() const;
    virtual void setVolatileValue(const QVariant &val);
    virtual void emitChangedValue() {}

    virtual void readSettings(const QSettings *settings);
    virtual void writeSettings(QSettings *settings) const;

    using SavedValueTransformation = std::function<QVariant(const QVariant &)>;
    void setFromSettingsTransformation(const SavedValueTransformation &transform);
    void setToSettingsTransformation(const SavedValueTransformation &transform);
    QVariant toSettingsValue(const QVariant &val) const;
    QVariant fromSettingsValue(const QVariant &val) const;

    virtual void apply();
    virtual void cancel();
    virtual void finish();
    bool isDirty() const;
    bool hasAction() const;

    class QTCREATOR_UTILS_EXPORT Data
    {
    public:
        // The (unique) address of the "owning" aspect's meta object is used as identifier.
        using ClassId = const void *;

        virtual ~Data() = default;

        Utils::Id id() const { return m_id; }
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
        Utils::Id m_id;
        ClassId m_classId = 0;
        std::function<Data *(const Data *)> m_cloner;
    };

    using DataCreator = std::function<Data *()>;
    using DataCloner = std::function<Data *(const Data *)>;
    using DataExtractor = std::function<void(Data *data, const MacroExpander *expander)>;

    Data::Ptr extractData(const MacroExpander *expander) const;

signals:
    void changed();
    void labelLinkActivated(const QString &link);

protected:
    QLabel *label() const;
    void setupLabel();
    void addLabeledItem(LayoutBuilder &builder, QWidget *widget);

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
        addDataExtractorHelper([aspect, p, q](Data *data, const MacroExpander *) {
            static_cast<DataClass *>(data)->*q = (aspect->*p)();
        });
    }

    template <typename AspectClass, typename DataClass, typename Type>
    void addDataExtractor(AspectClass *aspect,
                          Type(AspectClass::*p)(const MacroExpander *) const,
                          Type DataClass::*q) {
        setDataCreatorHelper([] {
            return new DataClass;
        });
        setDataClonerHelper([](const Data *data) {
            return new DataClass(*static_cast<const DataClass *>(data));
        });
        addDataExtractorHelper([aspect, p, q](Data *data, const MacroExpander *expander) {
            static_cast<DataClass *>(data)->*q = (aspect->*p)(expander);
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

class QTCREATOR_UTILS_EXPORT BoolAspect : public BaseAspect
{
    Q_OBJECT

public:
    explicit BoolAspect(const QString &settingsKey = QString());
    ~BoolAspect() override;

    struct Data : BaseAspect::Data
    {
        bool value;
    };

    void addToLayout(LayoutBuilder &builder) override;

    QAction *action() override;

    QVariant volatileValue() const override;
    void setVolatileValue(const QVariant &val) override;
    void emitChangedValue() override;

    bool value() const;
    void setValue(bool val);
    void setDefaultValue(bool val);

    enum class LabelPlacement { AtCheckBox, AtCheckBoxWithoutDummyLabel, InExtraLabel };
    void setLabel(const QString &labelText,
                  LabelPlacement labelPlacement = LabelPlacement::InExtraLabel);
    void setLabelPlacement(LabelPlacement labelPlacement);
    void setHandlesGroup(QGroupBox *box);

signals:
    void valueChanged(bool newValue);
    void volatileValueChanged(bool newValue);

private:
    std::unique_ptr<Internal::BoolAspectPrivate> d;
};

class QTCREATOR_UTILS_EXPORT SelectionAspect : public BaseAspect
{
    Q_OBJECT

public:
    SelectionAspect();
    ~SelectionAspect() override;

    void addToLayout(LayoutBuilder &builder) override;
    QVariant volatileValue() const override;
    void setVolatileValue(const QVariant &val) override;
    void finish() override;

    int value() const;
    void setValue(int val);
    void setStringValue(const QString &val);
    void setDefaultValue(int val);
    void setDefaultValue(const QString &val);

    QString stringValue() const;
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

signals:
    void volatileValueChanged(int newValue);

private:
    std::unique_ptr<Internal::SelectionAspectPrivate> d;
};

class QTCREATOR_UTILS_EXPORT MultiSelectionAspect : public BaseAspect
{
    Q_OBJECT

public:
    MultiSelectionAspect();
    ~MultiSelectionAspect() override;

    void addToLayout(LayoutBuilder &builder) override;

    enum class DisplayStyle { ListView };
    void setDisplayStyle(DisplayStyle style);

    QStringList value() const;
    void setValue(const QStringList &val);

    QStringList allValues() const;
    void setAllValues(const QStringList &val);

private:
    std::unique_ptr<Internal::MultiSelectionAspectPrivate> d;
};

class QTCREATOR_UTILS_EXPORT StringAspect : public BaseAspect
{
    Q_OBJECT

public:
    StringAspect();
    ~StringAspect() override;

    struct Data : BaseAspect::Data
    {
        QString value;
        Utils::FilePath filePath;
    };

    void addToLayout(LayoutBuilder &builder) override;

    QVariant volatileValue() const override;
    void setVolatileValue(const QVariant &val) override;
    void emitChangedValue() override;

    // Hook between UI and StringAspect:
    using ValueAcceptor = std::function<Utils::optional<QString>(const QString &, const QString &)>;
    void setValueAcceptor(ValueAcceptor &&acceptor);
    QString value() const;
    void setValue(const QString &val);
    void setDefaultValue(const QString &val);

    void setShowToolTipOnLabel(bool show);

    void setDisplayFilter(const std::function<QString (const QString &)> &displayFilter);
    void setPlaceHolderText(const QString &placeHolderText);
    void setHistoryCompleter(const QString &historyCompleterKey);
    void setExpectedKind(const Utils::PathChooser::Kind expectedKind);
    void setEnvironmentChange(const Utils::EnvironmentChange &change);
    void setBaseFileName(const Utils::FilePath &baseFileName);
    void setUndoRedoEnabled(bool readOnly);
    void setAcceptRichText(bool acceptRichText);
    void setMacroExpanderProvider(const Utils::MacroExpanderProvider &expanderProvider);
    void setUseGlobalMacroExpander();
    void setUseResetButton();
    void setValidationFunction(const Utils::FancyLineEdit::ValidationFunction &validator);
    void setOpenTerminalHandler(const std::function<void()> &openTerminal);
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

    Utils::FilePath filePath() const;
    void setFilePath(const Utils::FilePath &value);

    PathChooser *pathChooser() const; // Avoid to use.

signals:
    void checkedChanged();
    void valueChanged(const QString &newValue);

protected:
    void update();

    std::unique_ptr<Internal::StringAspectPrivate> d;
};

class QTCREATOR_UTILS_EXPORT IntegerAspect : public BaseAspect
{
    Q_OBJECT

public:
    IntegerAspect();
    ~IntegerAspect() override;

    void addToLayout(LayoutBuilder &builder) override;

    QVariant volatileValue() const override;
    void setVolatileValue(const QVariant &val) override;

    qint64 value() const;
    void setValue(qint64 val);
    void setDefaultValue(qint64 defaultValue);

    void setRange(qint64 min, qint64 max);
    void setLabel(const QString &label); // FIXME: Use setLabelText
    void setPrefix(const QString &prefix);
    void setSuffix(const QString &suffix);
    void setDisplayIntegerBase(int base);
    void setDisplayScaleFactor(qint64 factor);
    void setSpecialValueText(const QString &specialText);
    void setSingleStep(qint64 step);

    struct Data : BaseAspect::Data { qint64 value = 0; };

private:
    std::unique_ptr<Internal::IntegerAspectPrivate> d;
};

class QTCREATOR_UTILS_EXPORT DoubleAspect : public BaseAspect
{
    Q_OBJECT

public:
    DoubleAspect();
    ~DoubleAspect() override;

    void addToLayout(LayoutBuilder &builder) override;

    QVariant volatileValue() const override;
    void setVolatileValue(const QVariant &val) override;

    double value() const;
    void setValue(double val);
    void setDefaultValue(double defaultValue);

    void setRange(double min, double max);
    void setPrefix(const QString &prefix);
    void setSuffix(const QString &suffix);
    void setSpecialValueText(const QString &specialText);
    void setSingleStep(double step);

private:
    std::unique_ptr<Internal::DoubleAspectPrivate> d;
};

class QTCREATOR_UTILS_EXPORT TriState
{
    enum Value { EnabledValue, DisabledValue, DefaultValue };
    explicit TriState(Value v) : m_value(v) {}

public:
    TriState() = default;

    QVariant toVariant() const { return int(m_value); }
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
    TriStateAspect(
            const QString onString = tr("Enable"),
            const QString &offString = tr("Disable"),
            const QString &defaultString = tr("Leave at Default"));

    TriState value() const;
    void setValue(TriState setting);
    void setDefaultValue(TriState setting);
};

class QTCREATOR_UTILS_EXPORT StringListAspect : public BaseAspect
{
    Q_OBJECT

public:
    StringListAspect();
    ~StringListAspect() override;

    void addToLayout(LayoutBuilder &builder) override;

    QStringList value() const;
    void setValue(const QStringList &val);

    void appendValue(const QString &value, bool allowDuplicates = true);
    void removeValue(const QString &value);
    void appendValues(const QStringList &values, bool allowDuplicates = true);
    void removeValues(const QStringList &values);

private:
    std::unique_ptr<Internal::StringListAspectPrivate> d;
};

class QTCREATOR_UTILS_EXPORT IntegersAspect : public BaseAspect
{
    Q_OBJECT

public:
    IntegersAspect();
    ~IntegersAspect() override;

    void addToLayout(LayoutBuilder &builder) override;
    void emitChangedValue() override;

    QList<int> value() const;
    void setValue(const QList<int> &value);
    void setDefaultValue(const QList<int> &value);

signals:
    void valueChanged(const QList<int> &values);
};

class QTCREATOR_UTILS_EXPORT TextDisplay : public BaseAspect
{
    Q_OBJECT

public:
    TextDisplay(const QString &message = {},
                Utils::InfoLabel::InfoType type = Utils::InfoLabel::None);
    ~TextDisplay() override;

    void addToLayout(LayoutBuilder &builder) override;

    void setIconType(Utils::InfoLabel::InfoType t);
    void setText(const QString &message);

private:
    std::unique_ptr<Internal::TextDisplayPrivate> d;
};

class QTCREATOR_UTILS_EXPORT AspectContainerData
{
public:
    AspectContainerData() = default;

    const BaseAspect::Data *aspect(Utils::Id instanceId) const;
    const BaseAspect::Data *aspect(Utils::BaseAspect::Data::ClassId classId) const;

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

    void registerAspect(BaseAspect *aspect);
    void registerAspects(const AspectContainer &aspects);

    template <class Aspect, typename ...Args>
    Aspect *addAspect(Args && ...args)
    {
        auto aspect = new Aspect(args...);
        registerAspect(aspect);
        return aspect;
    }

    void fromMap(const QVariantMap &map);
    void toMap(QVariantMap &map) const;

    void readSettings(QSettings *settings);
    void writeSettings(QSettings *settings) const;

    void setSettingsGroup(const QString &groupKey);
    void setSettingsGroups(const QString &groupKey, const QString &subGroupKey);

    void apply();
    void cancel();
    void finish();

    void reset();
    bool equals(const AspectContainer &other) const;
    void copyFrom(const AspectContainer &other);
    void setAutoApply(bool on);
    void setOwnsSubAspects(bool on);
    bool isDirty() const;

    template <typename T> T *aspect() const
    {
        for (BaseAspect *aspect : aspects())
            if (T *result = qobject_cast<T *>(aspect))
                return result;
        return nullptr;
    }

    BaseAspect *aspect(Utils::Id id) const;

    template <typename T> T *aspect(Utils::Id id) const
    {
        return qobject_cast<T*>(aspect(id));
    }

    void forEachAspect(const std::function<void(BaseAspect *)> &run) const;

    const QList<BaseAspect *> &aspects() const;

    using const_iterator = QList<BaseAspect *>::const_iterator;
    using value_type = QList<BaseAspect *>::value_type;

    const_iterator begin() const;
    const_iterator end() const;

signals:
    void applied();
    void fromMapFinished();

private:
    std::unique_ptr<Internal::AspectContainerPrivate> d;
};

} // namespace Utils
