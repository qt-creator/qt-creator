// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljssettings.h"

#include "qmlformatsettings.h"
#include "qmljssettings.h"
#include "qmljsqtstylecodeformatter.h"
#include "qmljsindenter.h"
#include "qmljstoolsconstants.h"
#include "qmljstoolsinternalconstants.h"
#include "qmljstoolstr.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <cplusplus/Overview.h>

#include <qmljseditor/qmljseditorconstants.h>

#include <projectexplorer/editorconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projecttree.h>

#include <texteditor/codestyleeditor.h>
#include <texteditor/codestyleselectorwidget.h>
#include <texteditor/codestylepool.h>
#include <texteditor/command.h>
#include <texteditor/displaysettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/formattexteditor.h>
#include <texteditor/icodestylepreferencesfactory.h>
#include <texteditor/indenter.h>
#include <texteditor/snippets/snippeteditor.h>
#include <texteditor/snippets/snippetprovider.h>
#include <texteditor/tabsettings.h>
#include <texteditor/codestylepool.h>

#include <utils/aspects.h>
#include <utils/commandline.h>
#include <utils/filepath.h>
#include <utils/layoutbuilder.h>
#include <utils/mimeconstants.h>
#include <utils/mimeutils.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/shutdownguard.h>

#include <QAbstractItemView>
#include <QAbstractTableModel>
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSet>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QVBoxLayout>

using namespace std::chrono_literals;
using namespace TextEditor;
using namespace QmlJSTools::Internal;
using namespace Utils;

namespace QmlJSTools {

const char idKey[] = "QmlJSGlobal";
const char lineLengthKey[] = "LineLength";
const char qmlformatIniContentKey[] = "QmlFormatIniContent";
const char formatterKey[] = "Formatter";
const char customFormatterPathKey[] = "CustomFormatterPath";
const char customFormatterArgumentsKey[] = "CustomFormatterArguments";

// QmlJSCodeStyleSettings

QmlJSCodeStyleSettings::QmlJSCodeStyleSettings() = default;

void QmlJSCodeStyleSettings::toMap(Store &map) const
{
    map.insert(formatterKey, formatter);
    map.insert(lineLengthKey, lineLength);
    map.insert(qmlformatIniContentKey, qmlformatIniContent);
    map.insert(customFormatterPathKey, customFormatterPath.toUrlishString());
    map.insert(customFormatterArgumentsKey, customFormatterArguments);
}

void QmlJSCodeStyleSettings::fromMap(const Store &map)
{
    lineLength = map.value(lineLengthKey, lineLength).toInt();
    qmlformatIniContent = map.value(qmlformatIniContentKey, qmlformatIniContent).toString();
    formatter = static_cast<Formatter>(map.value(formatterKey, formatter).toInt());
    customFormatterPath = Utils::FilePath::fromString(map.value(customFormatterPathKey).toString());
    customFormatterArguments = map.value(customFormatterArgumentsKey).toString();
}

bool QmlJSCodeStyleSettings::equals(const QmlJSCodeStyleSettings &rhs) const
{
    return lineLength == rhs.lineLength && qmlformatIniContent == rhs.qmlformatIniContent
           && formatter == rhs.formatter && customFormatterPath == rhs.customFormatterPath
           && customFormatterArguments == rhs.customFormatterArguments;
}

QmlJSCodeStyleSettings QmlJSCodeStyleSettings::currentGlobalCodeStyle()
{
    QmlJSCodeStylePreferences *QmlJSCodeStylePreferences = globalQmlJSCodeStyle();
    QTC_ASSERT(QmlJSCodeStylePreferences, return QmlJSCodeStyleSettings());

    return QmlJSCodeStylePreferences->currentCodeStyleSettings();
}

TabSettingsData QmlJSCodeStyleSettings::currentGlobalTabSettings()
{
    QmlJSCodeStylePreferences *QmlJSCodeStylePreferences = globalQmlJSCodeStyle();
    QTC_ASSERT(QmlJSCodeStylePreferences, return TabSettingsData());

    return QmlJSCodeStylePreferences->currentTabSettings();
}

Id QmlJSCodeStyleSettings::settingsId()
{
    return Constants::QML_JS_CODE_STYLE_SETTINGS_ID;
}

// QmlCodeStyleWidgetBase

class QmlCodeStyleWidgetBase : public QWidget
{
    Q_OBJECT

public:
    explicit QmlCodeStyleWidgetBase(QWidget *parent) : QWidget(parent) {}

    virtual void setCodeStyleSettings(const QmlJSCodeStyleSettings &settings) = 0;
    virtual void setPreferences(QmlJSCodeStylePreferences *preferences) = 0;
    virtual void slotCurrentPreferencesChanged(TextEditor::ICodeStylePreferences *preferences) = 0;

signals:
    void settingsChanged(const QmlJSCodeStyleSettings &);
};

// FormatterSelectionWidget

class FormatterSelectionWidget : public QmlCodeStyleWidgetBase
{
public:
    explicit FormatterSelectionWidget(QWidget *parent);

    const Utils::SelectionAspect &selection() const { return m_formatterSelection; }
    Utils::SelectionAspect &selection() { return m_formatterSelection; }

    void setCodeStyleSettings(const QmlJSCodeStyleSettings &settings) override;
    void setPreferences(QmlJSCodeStylePreferences *preferences) override;
    void slotCurrentPreferencesChanged(TextEditor::ICodeStylePreferences *preferences) override;

private:
    void slotSettingsChanged();

    Utils::SelectionAspect m_formatterSelection;
    QmlJSCodeStylePreferences *m_preferences = nullptr;
};

FormatterSelectionWidget::FormatterSelectionWidget(QWidget *parent)
    : QmlCodeStyleWidgetBase(parent)
{
    m_formatterSelection.setDefaultValue(QmlJSCodeStyleSettings::Builtin);
    m_formatterSelection.setDisplayStyle(Utils::SelectionAspect::DisplayStyle::RadioButtons);
    m_formatterSelection.addOption(Tr::tr("Built-In Formatter [Deprecated]"));
    m_formatterSelection.addOption(Tr::tr("QmlFormat [LSP]"));
    m_formatterSelection.addOption(Tr::tr("Custom Formatter [Must be qmlformat compatible]"));
    m_formatterSelection.setLabelText(Tr::tr("Formatter"));

    connect(&m_formatterSelection, &Utils::SelectionAspect::changed,
            this, &FormatterSelectionWidget::slotSettingsChanged);

    using namespace Layouting;
    Column {
        Group {
            title(Tr::tr("Formatter Selection")),
            Column { m_formatterSelection, br },
        },
        noMargin,
    }.attachTo(this);
}

void FormatterSelectionWidget::setCodeStyleSettings(const QmlJSCodeStyleSettings &settings)
{
    if (settings.formatter != m_formatterSelection.value())
        m_formatterSelection.setValue(settings.formatter);
}

void FormatterSelectionWidget::setPreferences(QmlJSCodeStylePreferences *preferences)
{
    if (m_preferences == preferences)
        return;

    slotCurrentPreferencesChanged(preferences);

    if (m_preferences) {
        disconnect(m_preferences, &QmlJSCodeStylePreferences::currentValueChanged, this, nullptr);
        disconnect(m_preferences, &QmlJSCodeStylePreferences::currentPreferencesChanged,
                   this, &FormatterSelectionWidget::slotCurrentPreferencesChanged);
    }
    m_preferences = preferences;
    if (m_preferences) {
        setCodeStyleSettings(m_preferences->currentCodeStyleSettings());
        connect(m_preferences, &QmlJSCodeStylePreferences::currentValueChanged, this, [this] {
            setCodeStyleSettings(m_preferences->currentCodeStyleSettings());
        });
        connect(m_preferences, &QmlJSCodeStylePreferences::currentPreferencesChanged,
                this, &FormatterSelectionWidget::slotCurrentPreferencesChanged);
    }
}

void FormatterSelectionWidget::slotCurrentPreferencesChanged(
    TextEditor::ICodeStylePreferences *preferences)
{
    QmlJSCodeStylePreferences *current = dynamic_cast<QmlJSCodeStylePreferences *>(
        preferences ? preferences->currentPreferences() : nullptr);
    setEnabled(current && !current->isReadOnly());
}

void FormatterSelectionWidget::slotSettingsChanged()
{
    QmlJSCodeStyleSettings settings = m_preferences ? m_preferences->currentCodeStyleSettings()
                                                    : QmlJSCodeStyleSettings::currentGlobalCodeStyle();
    settings.formatter = static_cast<QmlJSCodeStyleSettings::Formatter>(m_formatterSelection.value());
    emit settingsChanged(settings);
}

// BuiltinFormatterSettingsWidget

class BuiltinFormatterSettingsWidget final : public QmlCodeStyleWidgetBase
{
public:
    BuiltinFormatterSettingsWidget(QWidget *parent, FormatterSelectionWidget *selection)
        : QmlCodeStyleWidgetBase(parent)
        , m_tabSettingsWidget(new TabSettings)
        , m_formatterSelectionWidget(selection)
    {
        m_lineLength.setRange(0, 999);
        m_tabSettingsWidget->setParent(this);

        using namespace Layouting;
        Column {
            Group {
                title(Tr::tr("Built-in Formatter Settings")),
                Column {
                    m_tabSettingsWidget,
                    Group {
                        title(Tr::tr("Other Settings")),
                        Form {
                            Tr::tr("Line length:"), m_lineLength, br
                        }
                    }
                }
            },
            noMargin
        }.attachTo(this);

        connect(&m_lineLength, &IntegerAspect::changed,
                this, &BuiltinFormatterSettingsWidget::slotSettingsChanged);
    }

    void setCodeStyleSettings(const QmlJSCodeStyleSettings &settings) override;
    void setPreferences(QmlJSCodeStylePreferences *preferences) override;
    void slotCurrentPreferencesChanged(ICodeStylePreferences *preferences) override;

private:
    void slotSettingsChanged();
    void slotTabSettingsChanged(const TabSettingsData &settings);

    IntegerAspect m_lineLength;
    TabSettings *m_tabSettingsWidget;
    QmlJSCodeStylePreferences *m_preferences = nullptr;
    FormatterSelectionWidget *m_formatterSelectionWidget;
};

void BuiltinFormatterSettingsWidget::setCodeStyleSettings(const QmlJSCodeStyleSettings &settings)
{
    QSignalBlocker blocker(this);
    m_lineLength.setValue(settings.lineLength);
}

void BuiltinFormatterSettingsWidget::setPreferences(QmlJSCodeStylePreferences *preferences)
{
    if (m_preferences == preferences)
        return;

    slotCurrentPreferencesChanged(preferences);

    if (m_preferences) {
        disconnect(m_preferences, &QmlJSCodeStylePreferences::currentValueChanged, this, nullptr);
        disconnect(m_preferences, &QmlJSCodeStylePreferences::currentPreferencesChanged,
                   this, &BuiltinFormatterSettingsWidget::slotCurrentPreferencesChanged);
        disconnect(m_preferences, &ICodeStylePreferences::currentTabSettingsChanged,
                   m_tabSettingsWidget, &TabSettings::setData);
        disconnect(m_tabSettingsWidget, &TabSettings::settingsChanged,
                   this, &BuiltinFormatterSettingsWidget::slotTabSettingsChanged);
    }
    m_preferences = preferences;
    if (m_preferences) {
        setCodeStyleSettings(m_preferences->currentCodeStyleSettings());
        connect(m_preferences, &QmlJSCodeStylePreferences::currentValueChanged, this, [this] {
            setCodeStyleSettings(m_preferences->currentCodeStyleSettings());
        });
        connect(m_preferences, &QmlJSCodeStylePreferences::currentPreferencesChanged,
                this, &BuiltinFormatterSettingsWidget::slotCurrentPreferencesChanged);
        m_tabSettingsWidget->setData(m_preferences->currentTabSettings());
        connect(m_preferences, &ICodeStylePreferences::currentTabSettingsChanged,
                m_tabSettingsWidget, &TabSettings::setData);
        connect(m_tabSettingsWidget, &TabSettings::settingsChanged,
                this, &BuiltinFormatterSettingsWidget::slotTabSettingsChanged);
    }
}

void BuiltinFormatterSettingsWidget::slotCurrentPreferencesChanged(ICodeStylePreferences *preferences)
{
    QmlJSCodeStylePreferences *current = dynamic_cast<QmlJSCodeStylePreferences *>(
        preferences ? preferences->currentPreferences() : nullptr);
    const bool enableWidgets = current && !current->isReadOnly() && m_formatterSelectionWidget
                               && m_formatterSelectionWidget->selection().value()
                                      == QmlJSCodeStyleSettings::Builtin;
    setEnabled(enableWidgets);
}

void BuiltinFormatterSettingsWidget::slotSettingsChanged()
{
    QmlJSCodeStyleSettings settings = m_preferences
                                          ? m_preferences->currentCodeStyleSettings()
                                          : QmlJSCodeStyleSettings::currentGlobalCodeStyle();
    settings.lineLength = m_lineLength.value();
    emit settingsChanged(settings);
}

void BuiltinFormatterSettingsWidget::slotTabSettingsChanged(const TabSettingsData &settings)
{
    if (!m_preferences)
        return;

    ICodeStylePreferences *current = m_preferences->currentPreferences();
    if (!current)
        return;

    current->setTabSettings(settings);
}

// CustomFormatterWidget

class CustomFormatterWidget : public QmlCodeStyleWidgetBase
{
public:
    CustomFormatterWidget(QWidget *parent, FormatterSelectionWidget *selection)
        : QmlCodeStyleWidgetBase(parent)
        , m_formatterSelectionWidget(selection)
    {
        m_customFormatterPath.setParent(this);
        m_customFormatterArguments.setParent(this);

        m_customFormatterPath.setPlaceHolderText(
                    QmlFormatSettings::instance().latestQmlFormatPath().toUrlishString());
        m_customFormatterPath.setLabelText(Tr::tr("Command:"));

        m_customFormatterArguments.setLabelText(Tr::tr("Arguments:"));
        m_customFormatterArguments.setDisplayStyle(StringAspect::LineEditDisplay);

        using namespace Layouting;
        Column {
            Group {
                title(Tr::tr("Custom Formatter Configuration")),
                Column {
                    m_customFormatterPath, br,
                    m_customFormatterArguments, br,
                    st
                },
            },
            noMargin,
        }.attachTo(this);

        connect(&m_customFormatterPath, &BaseAspect::changed,
                this, &CustomFormatterWidget::slotSettingsChanged);
        connect(&m_customFormatterArguments, &BaseAspect::changed,
                this, &CustomFormatterWidget::slotSettingsChanged);
    }

    void setCodeStyleSettings(const QmlJSCodeStyleSettings &settings) override;
    void setPreferences(QmlJSCodeStylePreferences *preferences) override;
    void slotCurrentPreferencesChanged(ICodeStylePreferences *preferences) override;

private:
    void slotSettingsChanged();

    FilePathAspect m_customFormatterPath;
    StringAspect m_customFormatterArguments;
    FormatterSelectionWidget *m_formatterSelectionWidget = nullptr;
    QmlJSCodeStylePreferences *m_preferences = nullptr;
};

void CustomFormatterWidget::setCodeStyleSettings(const QmlJSCodeStyleSettings &settings)
{
    QSignalBlocker blocker(this);
    if (settings.customFormatterPath != m_customFormatterPath.expandedValue())
        m_customFormatterPath.setValue(settings.customFormatterPath);
    if (settings.customFormatterArguments != m_customFormatterArguments.value())
        m_customFormatterArguments.setValue(settings.customFormatterArguments);
}

void CustomFormatterWidget::setPreferences(QmlJSCodeStylePreferences *preferences)
{
    if (m_preferences == preferences)
        return;

    slotCurrentPreferencesChanged(preferences);

    if (m_preferences) {
        disconnect(m_preferences, &QmlJSCodeStylePreferences::currentValueChanged, this, nullptr);
        disconnect(m_preferences, &QmlJSCodeStylePreferences::currentPreferencesChanged,
                   this, &CustomFormatterWidget::slotCurrentPreferencesChanged);
    }
    m_preferences = preferences;
    if (m_preferences) {
        setCodeStyleSettings(m_preferences->currentCodeStyleSettings());
        connect(m_preferences, &QmlJSCodeStylePreferences::currentValueChanged, this, [this] {
            setCodeStyleSettings(m_preferences->currentCodeStyleSettings());
        });
        connect(m_preferences, &QmlJSCodeStylePreferences::currentPreferencesChanged,
                this, &CustomFormatterWidget::slotCurrentPreferencesChanged);
    }
}

void CustomFormatterWidget::slotCurrentPreferencesChanged(ICodeStylePreferences *preferences)
{
    QmlJSCodeStylePreferences *current = dynamic_cast<QmlJSCodeStylePreferences *>(
        preferences ? preferences->currentPreferences() : nullptr);
    const bool enableWidgets = current && !current->isReadOnly() && m_formatterSelectionWidget
                               && m_formatterSelectionWidget->selection().value()
                                      == QmlJSCodeStyleSettings::Custom;
    setEnabled(enableWidgets);
}

void CustomFormatterWidget::slotSettingsChanged()
{
    QmlJSCodeStyleSettings settings = m_preferences ? m_preferences->currentCodeStyleSettings()
                                                    : QmlJSCodeStyleSettings::currentGlobalCodeStyle();
    if (m_customFormatterPath.value().isEmpty()) {
        m_customFormatterPath.setValue(
            QmlFormatSettings::instance().latestQmlFormatPath().toUrlishString());
    }
    settings.customFormatterPath = m_customFormatterPath.expandedValue();
    settings.customFormatterArguments = m_customFormatterArguments.value();
    emit settingsChanged(settings);
}

// QmlFormatOptionsModel

class QmlFormatOptionsModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column : int { Name = 0, Value };

    struct Option {
        QString name;
        QVariant value;
        QString hint;
        bool hidden = false;

        bool isBool() const { return hint == QString::fromUtf8(QMetaType::fromType<bool>().name()); }
        bool isInt() const { return hint == QString::fromUtf8(QMetaType::fromType<int>().name()); }
        bool isString() const { return hint == QString::fromUtf8(QMetaType::fromType<QString>().name()); }
        bool isStringList() const { return !isBool() && !isInt() && !isString() && !isNull(); }
        bool isNull() const { return hint.isEmpty(); }
    };

    explicit QmlFormatOptionsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void setOptionsFromJson(const QJsonDocument &doc);
    QString writeGlobalQmlFormatIniFile() const;
    void loadGlobalQmlFormatIniFile();

    const QList<Option> &options() const { return m_options; }

private:
    QList<Option> m_options;
};

QmlFormatOptionsModel::QmlFormatOptionsModel(QObject *parent)
    : QAbstractTableModel(parent)
{}

int QmlFormatOptionsModel::rowCount(const QModelIndex &) const { return m_options.size(); }
int QmlFormatOptionsModel::columnCount(const QModelIndex &) const { return 2; }

QVariant QmlFormatOptionsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_options.size())
        return QVariant();

    const Option &option = m_options.at(index.row());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case Column::Name: return option.name;
        case Column::Value: return option.isBool() ? QString() : option.value;
        }
    } else if (role == Qt::EditRole) {
        switch (index.column()) {
        case Column::Name: return option.name;
        case Column::Value: return option.value;
        }
    } else if (role == Qt::CheckStateRole && index.column() == Column::Value && option.isBool()) {
        return option.value.toBool() ? Qt::Checked : Qt::Unchecked;
    } else if (role == Qt::ForegroundRole && option.hidden) {
        return QColor(Qt::gray);
    } else if (role == Qt::ToolTipRole && option.hidden) {
        return Tr::tr("This option was found in the INI file but is not a standard qmlformat option.");
    }

    return QVariant();
}

QVariant QmlFormatOptionsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case Column::Name: return Tr::tr("Option");
        case Column::Value: return Tr::tr("Value");
        }
    }
    return QVariant();
}

bool QmlFormatOptionsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() >= m_options.size() || index.column() != Column::Value)
        return false;

    if (role == Qt::EditRole) {
        m_options[index.row()].value = value;
        emit dataChanged(index, index);
        return true;
    } else if (role == Qt::CheckStateRole && m_options[index.row()].isBool()) {
        m_options[index.row()].value = (value.toInt() == Qt::Checked);
        emit dataChanged(index, index);
        return true;
    }
    return false;
}

Qt::ItemFlags QmlFormatOptionsModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if (index.column() == Column::Value) {
        flags |= Qt::ItemIsEditable;
        if (index.row() < m_options.size() && m_options[index.row()].isBool())
            flags |= Qt::ItemIsUserCheckable;
    }
    return flags;
}

void QmlFormatOptionsModel::setOptionsFromJson(const QJsonDocument &doc)
{
    beginResetModel();
    m_options.clear();

    if (doc.isObject()) {
        for (const QJsonValue &val : doc.object()["options"].toArray()) {
            if (!val.isObject())
                continue;
            QJsonObject obj = val.toObject();
            m_options.append({obj["name"].toString(), obj["value"].toVariant(),
                              obj["hint"].toString()});
        }
    }

    endResetModel();
}

QString QmlFormatOptionsModel::writeGlobalQmlFormatIniFile() const
{
    QSettings settings(QmlFormatSettings::instance().globalQmlFormatIniFile().toFSPathString(),
                       QSettings::IniFormat);
    settings.clear();
    for (const Option &option : m_options)
        settings.setValue(option.name, option.value);
    settings.sync();

    QFile file(QmlFormatSettings::instance().globalQmlFormatIniFile().toFSPathString());
    QTC_CHECK(file.open(QIODevice::ReadOnly));
    return QString::fromUtf8(file.readAll());
}

void QmlFormatOptionsModel::loadGlobalQmlFormatIniFile()
{
    beginResetModel();

    QSettings settings(QmlFormatSettings::instance().globalQmlFormatIniFile().toFSPathString(),
                       QSettings::IniFormat);

    QSet<QString> foundOptions;
    for (Option &option : m_options) {
        if (settings.contains(option.name)) {
            option.value = settings.value(option.name);
            foundOptions.insert(option.name);
        }
    }
    for (const QString &key : settings.allKeys()) {
        if (!foundOptions.contains(key))
            m_options.append({key, settings.value(key), QString(), true});
    }
    std::sort(m_options.begin(), m_options.end(),
              [](const Option &a, const Option &b) { return a.name < b.name; });

    endResetModel();
}

// QmlFormatOptionsDelegate

class QmlFormatOptionsDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit QmlFormatOptionsDelegate(QmlFormatOptionsModel *model, QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;

private:
    QmlFormatOptionsModel *m_model;
};

QmlFormatOptionsDelegate::QmlFormatOptionsDelegate(QmlFormatOptionsModel *model, QObject *parent)
    : QStyledItemDelegate(parent), m_model(model)
{}

QWidget *QmlFormatOptionsDelegate::createEditor(QWidget *parent,
                                                 const QStyleOptionViewItem &,
                                                 const QModelIndex &index) const
{
    if (index.column() != 1)
        return nullptr;
    const auto &opts = m_model->options();
    if (index.row() >= opts.size())
        return nullptr;

    const QmlFormatOptionsModel::Option &opt = opts.at(index.row());
    if (opt.isInt()) {
        auto *spinBox = new QSpinBox(parent);
        spinBox->setRange(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
        return spinBox;
    }
    if (opt.isString())
        return new QLineEdit(parent);
    if (opt.isStringList()) {
        auto *comboBox = new QComboBox(parent);
        comboBox->addItems(opt.hint.split(','));
        return comboBox;
    }
    return nullptr;
}

void QmlFormatOptionsDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    const auto &opts = m_model->options();
    if (index.row() >= opts.size())
        return;
    const QmlFormatOptionsModel::Option &opt = opts.at(index.row());
    if (opt.isInt()) {
        if (auto *sb = qobject_cast<QSpinBox *>(editor))
            sb->setValue(opt.value.toInt());
    } else if (opt.isString()) {
        if (auto *le = qobject_cast<QLineEdit *>(editor))
            le->setText(opt.value.toString());
    } else if (opt.isStringList()) {
        if (auto *cb = qobject_cast<QComboBox *>(editor))
            cb->setCurrentText(opt.value.toString());
    }
}

void QmlFormatOptionsDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                             const QModelIndex &index) const
{
    const auto &opts = m_model->options();
    if (index.row() >= opts.size())
        return;
    const QmlFormatOptionsModel::Option &opt = opts.at(index.row());
    QVariant value;
    if (opt.isInt()) {
        if (auto *sb = qobject_cast<QSpinBox *>(editor))
            value = sb->value();
    } else if (opt.isString()) {
        if (auto *le = qobject_cast<QLineEdit *>(editor))
            value = le->text();
    } else if (opt.isStringList()) {
        if (auto *cb = qobject_cast<QComboBox *>(editor))
            value = cb->currentText();
    }
    if (value.isValid())
        model->setData(index, value, Qt::EditRole);
}

// QmlFormatSettingsWidget

class QmlFormatSettingsWidget : public QmlCodeStyleWidgetBase
{
public:
    QmlFormatSettingsWidget(QWidget *parent, FormatterSelectionWidget *selection);

    void setCodeStyleSettings(const QmlJSCodeStyleSettings &settings) override;
    void setPreferences(QmlJSCodeStylePreferences *preferences) override;
    void slotCurrentPreferencesChanged(TextEditor::ICodeStylePreferences *preferences) override;

private:
    void slotSettingsChanged();
    void initVersion();
    void initOptions();
    void resetOptions();
    void generateFallbackJson();

    QTableView *m_optionsTableView;
    QmlFormatOptionsModel *m_optionsModel;
    QmlFormatOptionsDelegate *m_optionsDelegate;
    QPushButton *m_deployIniButton;
    QPushButton *m_tableResetButton;
    QLabel *m_versionLabel;
    FormatterSelectionWidget *m_formatterSelectionWidget = nullptr;
    QmlJSCodeStylePreferences *m_preferences = nullptr;
    QJsonDocument m_fallbackJson;
};

QmlFormatSettingsWidget::QmlFormatSettingsWidget(QWidget *parent, FormatterSelectionWidget *selection)
    : QmlCodeStyleWidgetBase(parent)
    , m_optionsTableView(new QTableView())
    , m_optionsModel(new QmlFormatOptionsModel(this))
    , m_optionsDelegate(new QmlFormatOptionsDelegate(m_optionsModel, this))
    , m_deployIniButton(new QPushButton(Tr::tr("Deploy INI File to Current Project")))
    , m_tableResetButton(new QPushButton(Tr::tr("Reset to Defaults")))
    , m_versionLabel(new QLabel())
    , m_formatterSelectionWidget(selection)
{
    generateFallbackJson();

    m_optionsTableView->setModel(m_optionsModel);
    m_optionsTableView->setItemDelegate(m_optionsDelegate);
    m_optionsTableView->horizontalHeader()->setStretchLastSection(false);
    m_optionsTableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_optionsTableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_optionsTableView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_optionsTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    QSizePolicy sp(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    sp.setHorizontalStretch(1);
    m_optionsTableView->setSizePolicy(sp);

    m_versionLabel->setOpenExternalLinks(true);

    using namespace Layouting;
    Column{
        Group{
            title(Tr::tr("Global qmlformat Configuration")),
            Column{
                Row{
                    m_versionLabel,
                    Label{
                        openExternalLinks(true),
                        text(
                            "<a href='https://doc.qt.io/qt/qtqml-tooling-qmlformat.html'>"
                            + Tr::tr("Open latest documentation") + "</a>"),
                    },
                    st,
                },
                m_optionsTableView,
                Row{
                    st,
                    m_deployIniButton,
                    m_tableResetButton,
                },
                Label{
                    wordWrap(true),
                    text(
                        "<i>"
                        + Tr::tr(
                            "Global formatting options are ignored by projects having "
                            "their own deployed .qmlformat.ini files.")
                        + "</i>"),
                },
            },
        },
        noMargin}
        .attachTo(this);

    initOptions();
    initVersion();

    connect(m_optionsModel, &QmlFormatOptionsModel::dataChanged,
            this, &QmlFormatSettingsWidget::slotSettingsChanged);
    connect(m_tableResetButton, &QPushButton::clicked,
            this, &QmlFormatSettingsWidget::resetOptions);
    connect(m_deployIniButton, &QPushButton::clicked, this, [this] {
        if (ProjectExplorer::Project * const p = ProjectExplorer::ProjectTree::currentProject()) {
            p->projectDirectory().pathAppended(".qmlformat.ini")
                .writeFileContents(m_optionsModel->writeGlobalQmlFormatIniFile().toUtf8());
        }
    });

    m_deployIniButton->setEnabled(ProjectExplorer::ProjectTree::currentProject());
    connect(ProjectExplorer::ProjectTree::instance(),
            &ProjectExplorer::ProjectTree::currentProjectChanged,
            this, [this] {
                m_deployIniButton->setEnabled(ProjectExplorer::ProjectTree::currentProject());
            });
}

void QmlFormatSettingsWidget::setCodeStyleSettings(const QmlJSCodeStyleSettings &settings)
{
    QSignalBlocker blocker(this);
    QmlFormatSettings::instance().globalQmlFormatIniFile()
        .writeFileContents(settings.qmlformatIniContent.toUtf8());
    m_optionsModel->loadGlobalQmlFormatIniFile();
}

void QmlFormatSettingsWidget::setPreferences(QmlJSCodeStylePreferences *preferences)
{
    if (m_preferences == preferences)
        return;

    slotCurrentPreferencesChanged(preferences);

    if (m_preferences) {
        disconnect(m_preferences, &QmlJSCodeStylePreferences::currentValueChanged, this, nullptr);
        disconnect(m_preferences, &QmlJSCodeStylePreferences::currentPreferencesChanged,
                   this, &QmlFormatSettingsWidget::slotCurrentPreferencesChanged);
    }
    m_preferences = preferences;
    if (m_preferences) {
        setCodeStyleSettings(m_preferences->currentCodeStyleSettings());
        connect(m_preferences, &QmlJSCodeStylePreferences::currentValueChanged, this, [this] {
            setCodeStyleSettings(m_preferences->currentCodeStyleSettings());
        });
        connect(m_preferences, &QmlJSCodeStylePreferences::currentPreferencesChanged,
                this, &QmlFormatSettingsWidget::slotCurrentPreferencesChanged);
    }
}

void QmlFormatSettingsWidget::slotCurrentPreferencesChanged(
    TextEditor::ICodeStylePreferences *preferences)
{
    auto *current = dynamic_cast<QmlJSCodeStylePreferences *>(
        preferences ? preferences->currentPreferences() : nullptr);
    const bool enableWidgets = current && !current->isReadOnly() && m_formatterSelectionWidget
                               && m_formatterSelectionWidget->selection().value()
                                      == QmlJSCodeStyleSettings::QmlFormat;
    setEnabled(enableWidgets);
}

void QmlFormatSettingsWidget::slotSettingsChanged()
{
    QmlJSCodeStyleSettings settings = m_preferences ? m_preferences->currentCodeStyleSettings()
                                                    : QmlJSCodeStyleSettings::currentGlobalCodeStyle();
    settings.qmlformatIniContent = m_optionsModel->writeGlobalQmlFormatIniFile();
    emit settingsChanged(settings);
}

void QmlFormatSettingsWidget::initVersion()
{
    using namespace Core;
    const FilePath &qmlFormatPath = QmlFormatSettings::instance().latestQmlFormatPath();
    if (qmlFormatPath.isEmpty()) {
        MessageManager::writeSilently(Tr::tr("qmlformat not found. No version."));
        m_versionLabel->setText("Unknown qmlformat version");
        return;
    }
    const FilePath executable = CommandLine(qmlFormatPath).executable();

    Process process;
    process.setCommand({executable, {"--version"}});
    process.setUtf8StdOutCodec();
    process.start();
    if (!process.waitForFinished(5s)) {
        MessageManager::writeFlashing(
            Tr::tr("Cannot run \"%1\" or some other error occurred. No version.")
                .arg(executable.toUserOutput()));
        m_versionLabel->setText("Unknown qmlformat version");
        return;
    }
    const QString errorText = process.readAllStandardError();
    if (!errorText.isEmpty()) {
        MessageManager::writeFlashing(
            Tr::tr("\"%1\": %2. No version.").arg(executable.toUserOutput(), errorText));
        m_versionLabel->setText("Unknown qmlformat version");
        return;
    }
    m_versionLabel->setText(process.readAllStandardOutput().trimmed());
}

void QmlFormatSettingsWidget::initOptions()
{
    using namespace Core;
    const FilePath &qmlFormatPath = QmlFormatSettings::instance().latestQmlFormatPath();
    if (qmlFormatPath.isEmpty()) {
        MessageManager::writeSilently(Tr::tr("qmlformat not found. Using fallback output options."));
        m_optionsModel->setOptionsFromJson(m_fallbackJson);
        return;
    }
    const FilePath executable = CommandLine(qmlFormatPath).executable();

    Process process;
    process.setCommand({executable, {"--output-options"}});
    process.setUtf8StdOutCodec();
    process.start();
    if (!process.waitForFinished(5s)) {
        MessageManager::writeFlashing(
            Tr::tr("Cannot run \"%1\" or some other error occurred. Using fallback output options.")
                .arg(executable.toUserOutput()));
        m_optionsModel->setOptionsFromJson(m_fallbackJson);
        return;
    }
    const QString errorText = process.readAllStandardError();
    if (!errorText.isEmpty()) {
        MessageManager::writeFlashing(
            Tr::tr("\"%1\": %2. Using fallback output options.")
                .arg(executable.toUserOutput(), errorText));
        m_optionsModel->setOptionsFromJson(m_fallbackJson);
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(process.readAllStandardOutput().toUtf8());
    if (doc.isNull() || !doc.isObject() || !doc.object().contains("options")) {
        MessageManager::writeFlashing(
            Tr::tr("Invalid JSON response from qmlformat. Using fallback output options."));
        m_optionsModel->setOptionsFromJson(m_fallbackJson);
        return;
    }
    m_optionsModel->setOptionsFromJson(doc);
}

void QmlFormatSettingsWidget::resetOptions()
{
    initOptions();
    slotSettingsChanged();
}

void QmlFormatSettingsWidget::generateFallbackJson()
{
    QJsonObject root;
    QJsonArray optionsArray;
    auto addOption = [&](QLatin1StringView name, const QJsonValue &value, QLatin1StringView hint) {
        QJsonObject obj;
        obj["name"] = name;
        obj["value"] = value;
        obj["hint"] = hint;
        optionsArray.append(obj);
    };
    addOption(QLatin1String("NewlineType"), "native", QLatin1String("native,macos,unix,windows"));
    addOption(QLatin1String("MaxColumnWidth"), -1, QLatin1String(QMetaType::fromType<int>().name()));
    addOption(QLatin1String("UseTabs"), false, QLatin1String(QMetaType::fromType<bool>().name()));
    addOption(QLatin1String("NormalizeOrder"), false, QLatin1String(QMetaType::fromType<bool>().name()));
    addOption(QLatin1String("ObjectsSpacing"), false, QLatin1String(QMetaType::fromType<bool>().name()));
    addOption(QLatin1String("FunctionsSpacing"), false, QLatin1String(QMetaType::fromType<bool>().name()));
    addOption(QLatin1String("IndentWidth"), 4, QLatin1String(QMetaType::fromType<int>().name()));
    addOption(QLatin1String("SemicolonRule"), "always", QLatin1String("always,essential"));
    root[QStringLiteral("options")] = optionsArray;
    m_fallbackJson = QJsonDocument(root);
}

// QmlJSCodeStylePreferencesWidget

QmlJSCodeStylePreferencesWidget::QmlJSCodeStylePreferencesWidget(
    const QString &previewText, QWidget *parent)
    : TextEditor::CodeStyleWidget(parent)
    , m_formatterSelectionWidget(new FormatterSelectionWidget(this))
    , m_formatterSettingsStack(new QStackedWidget(this))
{
    m_formatterSettingsStack->insertWidget(QmlJSCodeStyleSettings::Builtin,
            new BuiltinFormatterSettingsWidget(this, m_formatterSelectionWidget));
    m_formatterSettingsStack->insertWidget(QmlJSCodeStyleSettings::QmlFormat,
            new QmlFormatSettingsWidget(this, m_formatterSelectionWidget));
    m_formatterSettingsStack->insertWidget(QmlJSCodeStyleSettings::Custom,
            new CustomFormatterWidget(this, m_formatterSelectionWidget));
    m_formatterSettingsStack->setContentsMargins({});

    m_formatterSelectionWidget->setContentsMargins({});

    for (const auto &formatterWidget :
         m_formatterSettingsStack->findChildren<QmlCodeStyleWidgetBase *>()) {
        formatterWidget->setContentsMargins({});
        connect(
            formatterWidget,
            &QmlCodeStyleWidgetBase::settingsChanged,
            this,
            &QmlJSCodeStylePreferencesWidget::slotSettingsChanged);
    }

    const int index = m_formatterSelectionWidget->selection().value();
    m_formatterSettingsStack->setCurrentIndex(index);

    m_previewTextEdit = new SnippetEditorWidget(this);
    m_previewTextEdit->setPlainText(previewText);
    QSizePolicy sp(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    sp.setHorizontalStretch(1);
    m_previewTextEdit->setSizePolicy(sp);
    decorateEditor(globalFontSettings().data());

    connect(&globalFontSettings(), &FontSettings::changed, this, [this] {
        decorateEditor(globalFontSettings().data());
    });

    connect(
        m_formatterSelectionWidget,
        &FormatterSelectionWidget::settingsChanged,
        [this](const QmlJSCodeStyleSettings &settings) {
            int index = m_formatterSelectionWidget->selection().volatileValue();
            if (index < 0 || index >= static_cast<int>(m_formatterSettingsStack->count()))
                return;

            m_formatterSettingsStack->setCurrentIndex(index);
            if (auto *current = dynamic_cast<QmlCodeStyleWidgetBase *>(
                    m_formatterSettingsStack->widget(index))) {
                current->slotCurrentPreferencesChanged(m_preferences);
            }
            slotSettingsChanged(settings);
        });

    using namespace Layouting;
    Row{Column{m_formatterSelectionWidget, br, m_formatterSettingsStack, st, noMargin},
        Column{
            Group{
                title(Tr::tr("Preview")),
                Column{
                    Label{
                        wordWrap(true),
                        text(
                            Tr::tr(
                                "Edit preview contents to see how the current settings "
                                "are applied to custom code snippets. Changes in the preview "
                                "do not affect the current settings.")),
                    },
                    m_previewTextEdit,
                    Row{
                        st,
                        PushButton{
                            text(Tr::tr("Reset to Original Preview Text")),
                            onClicked(
                                this,
                                [this, previewText]() {
                                    m_previewTextEdit->setPlainText(previewText);
                                }),
                        },
                        PushButton{
                            text(Tr::tr("Format Current Preview Text")),
                            onClicked(this, [this]() { this->updatePreview(); }),
                        },
                    },
                },
            },
        },
        noMargin}
        .attachTo(this);

    setVisualizeWhitespace(true);

    updatePreview();
}

void QmlJSCodeStylePreferencesWidget::setPreferences(QmlJSCodeStylePreferences *preferences)
{
    m_preferences = preferences;
    m_formatterSelectionWidget->setPreferences(preferences);
    for (const auto &formatterWidget :
         m_formatterSettingsStack->findChildren<QmlCodeStyleWidgetBase *>()) {
        formatterWidget->setPreferences(preferences);
    }
    if (m_preferences)
    {
        connect(m_preferences, &ICodeStylePreferences::currentTabSettingsChanged,
                this, &QmlJSCodeStylePreferencesWidget::updatePreview);
        connect(m_preferences, &QmlJSCodeStylePreferences::currentValueChanged,
                [this]{
                    m_formatterSettingsStack->setCurrentIndex(m_formatterSelectionWidget->selection().value());
                    updatePreview();
                });
    }
    updatePreview();
}

void QmlJSCodeStylePreferencesWidget::decorateEditor(const FontSettingsData &fontSettings)
{
    m_previewTextEdit->textDocument()->setFontSettings(fontSettings);
    SnippetProvider::decorateEditor(m_previewTextEdit,
                                    QmlJSEditor::Constants::QML_SNIPPETS_GROUP_ID);
}

void QmlJSCodeStylePreferencesWidget::setVisualizeWhitespace(bool on)
{
    DisplaySettingsData displaySettings = m_previewTextEdit->displaySettings();
    displaySettings.m_visualizeWhitespace = on;
    m_previewTextEdit->setDisplaySettings(displaySettings);
}

void QmlJSCodeStylePreferencesWidget::slotSettingsChanged(const QmlJSCodeStyleSettings &settings)
{
    if (!m_preferences)
        return;

    QmlJSCodeStylePreferences *current = dynamic_cast<QmlJSCodeStylePreferences*>(m_preferences->currentPreferences());
    if (!current)
        return;

    current->setCodeStyleSettings(settings);

    updatePreview();
}

void QmlJSCodeStylePreferencesWidget::updatePreview()
{
    switch (m_formatterSelectionWidget->selection().value()) {
    case QmlJSCodeStyleSettings::Builtin:
        builtInFormatterPreview();
        break;
    case QmlJSCodeStyleSettings::QmlFormat:
        qmlformatPreview();
        break;
    case QmlJSCodeStyleSettings::Custom:
        customFormatterPreview();
        break;
    }
}

void QmlJSCodeStylePreferencesWidget::builtInFormatterPreview()
{
    QTextDocument *doc = m_previewTextEdit->document();

    const TabSettingsData &ts = m_preferences
            ? m_preferences->currentTabSettings()
            : globalCodeStyle().tabSettings();
    m_previewTextEdit->textDocument()->setTabSettings(ts);
    CreatorCodeFormatter formatter(ts);
    formatter.invalidateCache(doc);

    QTextBlock block = doc->firstBlock();
    QTextCursor tc = m_previewTextEdit->textCursor();
    tc.beginEditBlock();
    while (block.isValid()) {
        m_previewTextEdit->textDocument()->indenter()->indentBlock(block, QChar::Null, ts);
        block = block.next();
    }
    tc.endEditBlock();
}

void QmlJSCodeStylePreferencesWidget::qmlformatPreview()
{
    using namespace Core;
    const Utils::FilePath &qmlFormatPath = QmlFormatSettings::instance().latestQmlFormatPath();
    if (qmlFormatPath.isEmpty()) {
        MessageManager::writeSilently("qmlformat not found.");
        return;
    }
    const Utils::CommandLine commandLine(qmlFormatPath);
    TextEditor::Command command;
    command.setExecutable(commandLine.executable());
    command.setProcessing(TextEditor::Command::FileProcessing);
    command.addOptions(commandLine.splitArguments());
    command.addOption("--inplace");
    command.addOption("%file");
    if (!command.isValid())
        return;
    TextEditor::TabSettingsData tabSettings;
    tabSettings.m_tabSize = 4;
    QSettings settings(
        QmlJSTools::QmlFormatSettings::globalQmlFormatIniFile().toUrlishString(),
        QSettings::IniFormat);
    if (settings.contains("IndentWidth"))
        tabSettings.m_indentSize = settings.value("IndentWidth").toInt();
    if (settings.contains("UseTabs"))
        tabSettings.m_tabPolicy = settings.value("UseTabs").toBool()
                                        ? TextEditor::TabSettingsData::TabPolicy::TabsOnlyTabPolicy
                                        : TextEditor::TabSettingsData::TabPolicy::SpacesOnlyTabPolicy;
    QString dummyFilePath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/dummy.qml";
    m_previewTextEdit->textDocument()->setFilePath(Utils::FilePath::fromString(dummyFilePath));
    m_previewTextEdit->textDocument()->setTabSettings(tabSettings);
    TextEditor::formatEditor(m_previewTextEdit, command);
}

void QmlJSCodeStylePreferencesWidget::customFormatterPreview()
{
    Utils::FilePath path = m_preferences->currentCodeStyleSettings().customFormatterPath;
    QStringList args = m_preferences->currentCodeStyleSettings()
                           .customFormatterArguments.split(" ", Qt::SkipEmptyParts);
    if (path.isEmpty()) {
        Core::MessageManager::writeSilently("Custom formatter not found.");
        return;
    }
    const Utils::CommandLine commandLine(path, args);
    TextEditor::Command command;
    command.setExecutable(commandLine.executable());
    command.setProcessing(TextEditor::Command::FileProcessing);
    command.addOptions(commandLine.splitArguments());
    command.addOption("--inplace");
    command.addOption("%file");
    if (!command.isValid())
        return;

    QString dummyFilePath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/dummy.qml";
    m_previewTextEdit->textDocument()->setFilePath(Utils::FilePath::fromString(dummyFilePath));
    TextEditor::formatEditor(m_previewTextEdit, command);
}

// QmlJSCodeStyleSettingsPageWidget

class QmlJSCodeStyleSettingsPageWidget : public Core::IOptionsPageWidget
{
public:
    QmlJSCodeStyleSettingsPageWidget()
    {
        QmlJSCodeStylePreferences *originalPreferences = globalQmlJSCodeStyle();
        m_preferences.setDelegatingPool(originalPreferences->delegatingPool());
        m_preferences.setCodeStyleSettings(originalPreferences->codeStyleSettings());
        m_preferences.setTabSettings(originalPreferences->tabSettings());
        m_preferences.setCurrentDelegate(originalPreferences->currentDelegate());
        m_preferences.setId(originalPreferences->id());

        auto vbox = new QVBoxLayout(this);
        vbox->addWidget(
            codeStyleFactory(QmlJSTools::Constants::QML_JS_SETTINGS_ID)
                ->createCodeStyleEditor({}, &m_preferences));
    }

    void apply() final
    {
        QmlJSCodeStylePreferences *originalPreferences = globalQmlJSCodeStyle();
        if (originalPreferences->codeStyleSettings() != m_preferences.codeStyleSettings()) {
            originalPreferences->setCodeStyleSettings(m_preferences.codeStyleSettings());
            originalPreferences->toSettings(QmlJSTools::Constants::QML_JS_SETTINGS_ID);
        }
        if (originalPreferences->tabSettings() != m_preferences.tabSettings()) {
            originalPreferences->setTabSettings(m_preferences.tabSettings());
            originalPreferences->toSettings(QmlJSTools::Constants::QML_JS_SETTINGS_ID);
        }
        if (originalPreferences->currentDelegate() != m_preferences.currentDelegate()) {
            originalPreferences->setCurrentDelegate(m_preferences.currentDelegate());
            originalPreferences->toSettings(QmlJSTools::Constants::QML_JS_SETTINGS_ID);
        }
    }

      QmlJSCodeStylePreferences m_preferences;
};

// QmlJSCodeStyleSettingsPage

QmlJSCodeStyleSettingsPage::QmlJSCodeStyleSettingsPage()
{
    setId(Constants::QML_JS_CODE_STYLE_SETTINGS_ID);
    setDisplayName(Tr::tr(Constants::QML_JS_CODE_STYLE_SETTINGS_NAME));
    setCategory(QmlJSEditor::Constants::SETTINGS_CATEGORY_QML);
    setWidgetCreator([] { return new QmlJSCodeStyleSettingsPageWidget; });
}

// QmlJsCodeStyleEditor

class QmlJsCodeStyleEditor final : public CodeStyleEditor
{
public:
    QmlJsCodeStyleEditor(const ICodeStylePreferencesFactory *factory,
                         const FilePath &projectFile,
                         ICodeStylePreferences *codeStyle,
                         QWidget *parent)
        : CodeStyleEditor{parent}
    {
        auto selector = new CodeStyleSelectorWidget{projectFile, this};
        selector->setCodeStyle(codeStyle);
        addSelector(selector);
        addInfoLabel();
        const QString text = QString::fromLatin1(Internal::previewText);
        if (projectFile.isEmpty()) {
            if (auto qmlJSPrefs = dynamic_cast<QmlJSCodeStylePreferences *>(codeStyle)) {
                auto widget = new QmlJSCodeStylePreferencesWidget(text, this);
                widget->setPreferences(qmlJSPrefs);
                addEditorWidget(widget);
            }
        } else {
            setupPreview(factory, projectFile, codeStyle,
                         text, QmlJSEditor::Constants::QML_SNIPPETS_GROUP_ID);
        }
    }
};

// QmlJSCodeStylePreferencesFactory

class QmlJSCodeStylePreferencesFactory final : public ICodeStylePreferencesFactory
{
public:
    QmlJSCodeStylePreferencesFactory()
        : ICodeStylePreferencesFactory(Constants::QML_JS_SETTINGS_ID)
    {}

private:
    CodeStyleEditor *createCodeStyleEditor(
            const FilePath &projectFile,
            ICodeStylePreferences *codeStyle,
            QWidget *parent) const final
    {
        return new QmlJsCodeStyleEditor{this, projectFile, codeStyle, parent};
    }

    QString displayName() final
    {
        return Tr::tr("Qt Quick");
    }

    ICodeStylePreferences *createCodeStyle() const final
    {
        return new QmlJSCodeStylePreferences;
    }

    Indenter *createIndenter(QTextDocument *doc) const final
    {
        return QmlJSEditor::createQmlJsIndenter(doc);
    }
};

// QmlJSToolsSettings

class QmlJSToolsSettings final : public QObject
{
public:
    QmlJSToolsSettings();
    ~QmlJSToolsSettings() final;

    QmlJSCodeStylePreferencesFactory m_factory;
    CodeStylePool m_pool{&m_factory, Constants::QML_JS_SETTINGS_ID};
    QmlJSCodeStylePreferences m_globalCodeStyle;
};

QmlJSToolsSettings::QmlJSToolsSettings()
{
    m_globalCodeStyle.setDelegatingPool(&m_pool);
    m_globalCodeStyle.setDisplayName(Tr::tr("Global", "Settings"));
    m_globalCodeStyle.setId(idKey);
    m_pool.addCodeStyle(&m_globalCodeStyle);
    registerCodeStyle(QmlJSTools::Constants::QML_JS_SETTINGS_ID, &m_globalCodeStyle);

    auto qtCodeStyle = new QmlJSCodeStylePreferences(this);
    qtCodeStyle->setId("qt");
    qtCodeStyle->setDisplayName(Tr::tr("Qt"));
    qtCodeStyle->setReadOnly(true);
    TabSettingsData qtTabSettings;
    qtTabSettings.m_tabPolicy = TabSettingsData::SpacesOnlyTabPolicy;
    qtTabSettings.m_tabSize = 4;
    qtTabSettings.m_indentSize = 4;
    qtTabSettings.m_continuationAlignBehavior = TabSettingsData::ContinuationAlignWithIndent;
    qtCodeStyle->setTabSettings(qtTabSettings);

    connect(&QmlFormatSettings::instance(), &QmlFormatSettings::qmlformatIniCreated,
            this, [](Utils::FilePath qmlformatIniPath) {
        QmlJSCodeStyleSettings s;
        s.lineLength = 80;
        const Utils::Result<QByteArray> fileContents = qmlformatIniPath.fileContents();
        if (fileContents)
            s.qmlformatIniContent = QString::fromUtf8(*fileContents);
        auto csPool = codeStylePool(QmlJSTools::Constants::QML_JS_SETTINGS_ID);
        QTC_ASSERT(csPool, return);
        auto builtInCodeStyles = csPool->builtInCodeStyles();
        for (auto codeStyle : builtInCodeStyles) {
            if (auto qtCodeStyle = dynamic_cast<QmlJSCodeStylePreferences *>(codeStyle))
                qtCodeStyle->setCodeStyleSettings(s);
        }
    });

    m_pool.addCodeStyle(qtCodeStyle);
    m_globalCodeStyle.setCurrentDelegate(qtCodeStyle);
    m_pool.loadCustomCodeStyles();
    m_globalCodeStyle.fromSettings(QmlJSTools::Constants::QML_JS_SETTINGS_ID);

    using namespace Utils::Constants;
    registerMimeTypeForLanguageId(QML_MIMETYPE, Constants::QML_JS_SETTINGS_ID);
    registerMimeTypeForLanguageId(QMLUI_MIMETYPE, Constants::QML_JS_SETTINGS_ID);
    registerMimeTypeForLanguageId(QBS_MIMETYPE, Constants::QML_JS_SETTINGS_ID);
    registerMimeTypeForLanguageId(QMLPROJECT_MIMETYPE, Constants::QML_JS_SETTINGS_ID);
    registerMimeTypeForLanguageId(QMLTYPES_MIMETYPE, Constants::QML_JS_SETTINGS_ID);
    registerMimeTypeForLanguageId(JS_MIMETYPE, Constants::QML_JS_SETTINGS_ID);
    registerMimeTypeForLanguageId(JSON_MIMETYPE, Constants::QML_JS_SETTINGS_ID);
}

QmlJSToolsSettings::~QmlJSToolsSettings()
{
    unregisterCodeStyle(QmlJSTools::Constants::QML_JS_SETTINGS_ID);
}

static QmlJSToolsSettings &toolsSettings()
{
    static GuardedObject<QmlJSToolsSettings> theQmlJSToolsSettings;
    return theQmlJSToolsSettings;
}

QmlJSCodeStylePreferences *globalQmlJSCodeStyle()
{
    return &toolsSettings().m_globalCodeStyle;
}

void Internal::setupQmlJSToolsSettings()
{
    (void) toolsSettings();
}

} // QmlJSTools::Internal

#include "qmljssettings.moc"
