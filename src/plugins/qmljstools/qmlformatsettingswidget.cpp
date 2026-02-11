// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlformatsettingswidget.h"

#include "coreplugin/messagemanager.h"
#include "qmlformatsettings.h"
#include "qmljscodestylesettings.h"
#include "qmljsformatterselectionwidget.h"
#include "qmljstoolstr.h"

#include <utils/commandline.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcprocess.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/project.h>

#include <QAbstractItemView>
#include <QAbstractTableModel>
#include <QColor>
#include <QCheckBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QSet>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStyledItemDelegate>
#include <QTableView>

#include <chrono>

using namespace std::chrono_literals;

using namespace ProjectExplorer;

namespace QmlJSTools {

class QmlFormatOptionsModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column : int {
        Name = 0,
        Value
    };

    struct Option {
        QString name;
        QVariant value;
        QString hint;
        bool hidden = false;

        bool isBool() const {
            return hint == QString::fromUtf8(QMetaType::fromType<bool>().name());
        }

        bool isInt() const {
            return hint == QString::fromUtf8(QMetaType::fromType<int>().name());
        }

        bool isString() const {
            return hint == QString::fromUtf8(QMetaType::fromType<QString>().name());
        }

        bool isStringList() const {
            return !isBool() && !isInt() && !isString() && !isNull();
        }

        bool isNull() const {
            return hint.isEmpty();
        }
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

signals:
    void dataChanged();

private:
    QList<Option> m_options;
};

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

QmlFormatOptionsModel::QmlFormatOptionsModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int QmlFormatOptionsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_options.size();
}

int QmlFormatOptionsModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 2; // Name and Value columns
}

QVariant QmlFormatOptionsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_options.size())
        return QVariant();

    const Option &option = m_options.at(index.row());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case Column::Name:
            return option.name;
        case Column::Value:
            // Hide text for boolean values, show empty string so checkbox is visible
            if (option.isBool()) // Boolean type
                return QString();
            return option.value;
        }
    } else if (role == Qt::EditRole) {
        switch (index.column()) {
        case Column::Name:
            return option.name;
        case Column::Value:
            return option.value;
        }
    } else if (role == Qt::CheckStateRole && index.column() == Column::Value && option.isBool()) {
        // For boolean values, provide check state
        return option.value.toBool() ? Qt::Checked : Qt::Unchecked;
    } else if (role == Qt::ForegroundRole && option.hidden) {
        // Show hidden options in gray
        return QColor(Qt::gray);
    } else if (role == Qt::ToolTipRole && option.hidden) {
        return Tr::tr(
            "This option was found in the INI file but is not a standard qmlformat option.");
    }

    return QVariant();
}

QVariant QmlFormatOptionsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case Column::Name:
            return Tr::tr("Option");
        case Column::Value:
            return Tr::tr("Value");
        }
    }
    return QVariant();
}

bool QmlFormatOptionsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() >= m_options.size())
        return false;

    if (index.column() == Column::Value) {
        if (role == Qt::EditRole) {
            m_options[index.row()].value = value;
            emit QAbstractTableModel::dataChanged(index, index);
            emit dataChanged();
            return true;
        } else if (role == Qt::CheckStateRole && m_options[index.row()].isBool()) {
            // Handle checkbox state changes for boolean values
            bool boolValue = (value.toInt() == Qt::Checked);
            m_options[index.row()].value = boolValue;
            emit QAbstractTableModel::dataChanged(index, index);
            emit dataChanged();
            return true;
        }
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

        // Add checkable flag for boolean values
        if (index.row() < m_options.size() && m_options[index.row()].isBool())
            flags |= Qt::ItemIsUserCheckable;
    }

    return flags;
}

void QmlFormatOptionsModel::setOptionsFromJson(const QJsonDocument &doc)
{
    beginResetModel();
    m_options.clear();

    if (!doc.isObject()) {
        endResetModel();
        return;
    }

    QJsonObject rootObj = doc.object();
    const QJsonArray optionsArray = rootObj["options"].toArray();

    for (const QJsonValue &optionValue : optionsArray) {
        if (!optionValue.isObject())
            continue;

        QJsonObject optionObj = optionValue.toObject();
        Option option;
        option.name = optionObj["name"].toString();
        option.value = optionObj["value"].toVariant();
        option.hint = optionObj["hint"].toString();
        m_options.append(option);
    }

    endResetModel();
}

QString QmlFormatOptionsModel::writeGlobalQmlFormatIniFile() const
{
    QSettings settings(
        QmlFormatSettings::instance().globalQmlFormatIniFile().toFSPathString(),
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
    // Parse INI content and update options
    beginResetModel();

    QSettings settings(
        QmlFormatSettings::instance().globalQmlFormatIniFile().toFSPathString(),
        QSettings::IniFormat);

    // Update existing options with INI values
    QSet<QString> foundOptions;
    for (Option &option : m_options) {
        if (settings.contains(option.name)) {
            option.value = settings.value(option.name);
            foundOptions.insert(option.name);
        }
    }

    // Add missing options as hidden
    for (const QString &key : settings.allKeys()) {
        if (!foundOptions.contains(key)) {
            Option hiddenOption;
            hiddenOption.name = key;
            hiddenOption.value = settings.value(key);
            hiddenOption.hint = QString(); // Unknown type for hidden options
            hiddenOption.hidden = true;
            m_options.append(hiddenOption);
        }
    }

    std::sort(m_options.begin(), m_options.end(), [](const Option &a, const Option &b){
        return a.name < b.name;
    });

    endResetModel();
}

QmlFormatOptionsDelegate::QmlFormatOptionsDelegate(QmlFormatOptionsModel *model, QObject *parent)
    : QStyledItemDelegate(parent), m_model(model)
{
}

QWidget *QmlFormatOptionsDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                                               const QModelIndex &index) const
{
    Q_UNUSED(option)

    if (index.column() != 1) // Only create editors for the value column
        return nullptr;

    const auto &options = m_model->options();
    if (index.row() >= options.size())
        return nullptr;

    const QmlFormatOptionsModel::Option &opt = options.at(index.row());

    if (opt.isInt()) {
        QSpinBox *spinBox = new QSpinBox(parent);
        spinBox->setRange(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
        return spinBox;
    }

    if (opt.isString()) {
        return new QLineEdit(parent);
    }

    if (opt.isStringList()) {
        QComboBox *comboBox = new QComboBox(parent);
        comboBox->addItems(opt.hint.split(','));
        return comboBox;
    }

    return nullptr;
}

void QmlFormatOptionsDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    const auto &options = m_model->options();
    if (index.row() >= options.size())
        return;

    const QmlFormatOptionsModel::Option &opt = options.at(index.row());

    if (opt.isInt()) {
        QSpinBox *spinBox = qobject_cast<QSpinBox*>(editor);
        if (spinBox)
            spinBox->setValue(opt.value.toInt());
    }

    if (opt.isString()) {
        QLineEdit *lineEdit = qobject_cast<QLineEdit*>(editor);
        if (lineEdit)
            lineEdit->setText(opt.value.toString());
    }

    if (opt.isStringList()) {
        QComboBox *comboBox = qobject_cast<QComboBox*>(editor);
        if (comboBox)
            comboBox->setCurrentText(opt.value.toString());
    }

    // No action needed for booleans, native checkbox handles this
}

void QmlFormatOptionsDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                           const QModelIndex &index) const
{
    const auto &options = m_model->options();
    if (index.row() >= options.size())
        return;

    const QmlFormatOptionsModel::Option &opt = options.at(index.row());

    QVariant value;

    if (opt.isInt()) {
        QSpinBox *spinBox = qobject_cast<QSpinBox*>(editor);
        if (spinBox)
            value = spinBox->value();
    }

    if (opt.isString()) {
        QLineEdit *lineEdit = qobject_cast<QLineEdit*>(editor);
        if (lineEdit)
            value = lineEdit->text();
    }

    if (opt.isStringList()) {
        QComboBox *comboBox = qobject_cast<QComboBox*>(editor);
        if (comboBox)
            value = comboBox->currentText();
    }

    // No action needed for boolean, native checkbox handles this through Qt::CheckStateRole

    if (value.isValid())
        model->setData(index, value, Qt::EditRole);
}

class QmlFormatSettingsWidget : public QmlCodeStyleWidgetBase
{
public:
    QmlFormatSettingsWidget(QWidget *parent, FormatterSelectionWidget *selection);

    void setCodeStyleSettings(const QmlJSCodeStyleSettings &s) override;
    void setPreferences(QmlJSCodeStylePreferences *preferences) override;
    void slotCurrentPreferencesChanged(TextEditor::ICodeStylePreferences* preferences) override;

private:
    void slotSettingsChanged();
    void initOptions();
    void resetOptions();
    void generateFallbackJson();
    QTableView *m_optionsTableView;
    QmlFormatOptionsModel *m_optionsModel;
    QmlFormatOptionsDelegate *m_optionsDelegate;
    QPushButton *m_deployIniButton;
    QPushButton *m_tableResetButton;
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

    using namespace Layouting;

    // clang-format off
    Column {
        Group {
            title(Tr::tr("Global qmlformat Configuration")),
            Column {
                m_optionsTableView,
                Row {
                    st,
                    m_deployIniButton,
                    m_tableResetButton,
                },
                Label {
                    wordWrap(true),
                    text("<i>" + Tr::tr("Global formatting options are ignored by projects having their own deployed .qmlformat.ini files.") + "</i>"),
                },
            },
        },
        noMargin
    }.attachTo(this);
    // clang-format on

    initOptions();

    connect(
        m_optionsModel,
        &QmlFormatOptionsModel::dataChanged,
        this,
        &QmlFormatSettingsWidget::slotSettingsChanged);

    connect(
        m_tableResetButton,
        &QPushButton::clicked,
        this,
        &QmlFormatSettingsWidget::resetOptions);

    connect(
        m_deployIniButton,
        &QPushButton::clicked,
        this,
        [this] {
            if (Project * const p = ProjectTree::currentProject()) {
                p->projectDirectory().pathAppended(".qmlformat.ini")
                    .writeFileContents(m_optionsModel->writeGlobalQmlFormatIniFile().toUtf8());
            }
        });

    m_deployIniButton->setEnabled(ProjectTree::currentProject());
    connect(ProjectTree::instance(), &ProjectTree::currentProjectChanged, this, [this] {
                m_deployIniButton->setEnabled(ProjectTree::currentProject());
            });
}

void QmlFormatSettingsWidget::setCodeStyleSettings(const QmlJSCodeStyleSettings &s)
{
    QSignalBlocker blocker(this);
    QmlFormatSettings::instance().globalQmlFormatIniFile().writeFileContents(s.qmlformatIniContent.toUtf8());
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
                this->setCodeStyleSettings(m_preferences->currentCodeStyleSettings());
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
                                      == QmlCodeStyleWidgetBase::QmlFormat;
    setEnabled(enableWidgets);
}

void QmlFormatSettingsWidget::slotSettingsChanged()
{
    QmlJSCodeStyleSettings settings = m_preferences ? m_preferences->currentCodeStyleSettings()
    : QmlJSCodeStyleSettings::currentGlobalCodeStyle();
    settings.qmlformatIniContent = m_optionsModel->writeGlobalQmlFormatIniFile();
    emit settingsChanged(settings);
}

void QmlFormatSettingsWidget::initOptions()
{
    using namespace Core;
    const Utils::FilePath &qmlFormatPath = QmlFormatSettings::instance().latestQmlFormatPath();
    if (qmlFormatPath.isEmpty()) {
        MessageManager::writeSilently(Tr::tr("QmlFormat not found. Using fallback output options."));
        m_optionsModel->setOptionsFromJson(m_fallbackJson);
        return;
    }
    const Utils::CommandLine commandLine(qmlFormatPath);
    const Utils::FilePath executable = commandLine.executable();

    Utils::Process process;
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
        //: %1=exceutable, %2=error
        MessageManager::writeFlashing(
            Tr::tr("\"%1\": %2. Using fallback output options.")
                .arg(executable.toUserOutput(), errorText));
        m_optionsModel->setOptionsFromJson(m_fallbackJson);
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(process.readAllStandardOutput().toUtf8());
    if (doc.isNull() || !doc.isObject() || !doc.object().contains("options")) {
        MessageManager::writeFlashing(Tr::tr("Invalid JSON response from qmlformat. Using fallback output options."));
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
    {
        QJsonObject obj;
        obj["name"] = "NewlineType";
        obj["value"] = "native";
        obj["hint"] = "native,macos,unix,windows";
        optionsArray.append(obj);
    }
    {
        QJsonObject obj;
        obj["name"] = "MaxColumnWidth";
        obj["value"] = -1;
        obj["hint"] = QMetaType::fromType<int>().name();
        optionsArray.append(obj);
    }
    {
        QJsonObject obj;
        obj["name"] = "UseTabs";
        obj["value"] = false;
        obj["hint"] = QMetaType::fromType<bool>().name();
        optionsArray.append(obj);
    }
    {
        QJsonObject obj;
        obj["name"] = "NormalizeOrder";
        obj["value"] = false;
        obj["hint"] = QMetaType::fromType<bool>().name();
        optionsArray.append(obj);
    }
    {
        QJsonObject obj;
        obj["name"] = "ObjectsSpacing";
        obj["value"] = false;
        obj["hint"] = QMetaType::fromType<bool>().name();
        optionsArray.append(obj);
    }
    {
        QJsonObject obj;
        obj["name"] = "FunctionsSpacing";
        obj["value"] = false;
        obj["hint"] = QMetaType::fromType<bool>().name();
        optionsArray.append(obj);
    }
    {
        QJsonObject obj;
        obj["name"] = "IndentWidth";
        obj["value"] = 4;
        obj["hint"] = QMetaType::fromType<int>().name();
        optionsArray.append(obj);
    }
    {
        QJsonObject obj;
        obj["name"] = "SemicolonRule";
        obj["value"] = "always";
        obj["hint"] = "always,essential";
        optionsArray.append(obj);
    }
    root[QStringLiteral("options")] = optionsArray;
    m_fallbackJson = QJsonDocument(root);
}

QmlCodeStyleWidgetBase *createQmlFormatSettingsWidget(QWidget *parent, FormatterSelectionWidget *selection)
{
    return new QmlFormatSettingsWidget(parent, selection);
}

} // namespace QmlJSTools

#include "qmlformatsettingswidget.moc"
