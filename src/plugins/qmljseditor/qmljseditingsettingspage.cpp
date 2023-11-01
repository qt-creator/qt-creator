// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljseditingsettingspage.h"
#include "qmljseditorconstants.h"
#include "qmljseditortr.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>

#include <qmljs/qmljscheck.h>
#include <qmljs/qmljsstaticanalysismessage.h>
#include <qmljstools/qmljstoolsconstants.h>

#include <utils/algorithm.h>
#include <utils/layoutbuilder.h>
#include <utils/macroexpander.h>
#include <utils/qtcsettings.h>
#include <utils/treemodel.h>
#include <utils/variablechooser.h>

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QTextStream>
#include <QTreeView>

const char AUTO_FORMAT_ON_SAVE[] = "QmlJSEditor.AutoFormatOnSave";
const char AUTO_FORMAT_ONLY_CURRENT_PROJECT[] = "QmlJSEditor.AutoFormatOnlyCurrentProject";
const char QML_CONTEXTPANE_KEY[] = "QmlJSEditor.ContextPaneEnabled";
const char QML_CONTEXTPANEPIN_KEY[] = "QmlJSEditor.ContextPanePinned";
const char FOLD_AUX_DATA[] = "QmlJSEditor.FoldAuxData";
const char USE_QMLLS[] = "QmlJSEditor.UseQmlls";
const char USE_LATEST_QMLLS[] = "QmlJSEditor.UseLatestQmlls";
const char DISABLE_BUILTIN_CODEMODEL[] = "QmlJSEditor.DisableBuiltinCodemodel";
const char UIQML_OPEN_MODE[] = "QmlJSEditor.openUiQmlMode";
const char FORMAT_COMMAND[] = "QmlJSEditor.formatCommand";
const char FORMAT_COMMAND_OPTIONS[] = "QmlJSEditor.formatCommandOptions";
const char CUSTOM_COMMAND[] = "QmlJSEditor.useCustomFormatCommand";
const char CUSTOM_ANALYZER[] = "QmlJSEditor.useCustomAnalyzer";
const char DISABLED_MESSAGES[] = "QmlJSEditor.disabledMessages";
const char DISABLED_MESSAGES_NONQUICKUI[] = "QmlJSEditor.disabledMessagesNonQuickUI";
const char DEFAULT_CUSTOM_FORMAT_COMMAND[] = "%{CurrentDocument:Project:QT_HOST_BINS}/qmlformat";

using namespace QmlJSEditor::Internal;
using namespace Utils;

namespace QmlJSEditor {

static QList<int> defaultDisabledMessages()
{
    static const QList<int> disabledByDefault = Utils::transform(
                QmlJS::Check::defaultDisabledMessages(),
                [](QmlJS::StaticAnalysis::Type t) { return int(t); });
    return disabledByDefault;
}

static QList<int> defaultDisabledMessagesNonQuickUi()
{
    static const QList<int> disabledForNonQuickUi = Utils::transform(
        QmlJS::Check::defaultDisabledMessagesForNonQuickUi(),
        [](QmlJS::StaticAnalysis::Type t){ return int(t); });
    return disabledForNonQuickUi;
}

void QmlJsEditingSettings::set()
{
    if (get() != *this)
        toSettings(Core::ICore::settings());
}

static QStringList intListToStringList(const QList<int> &list)
{
    return Utils::transform(list, [](int v) { return QString::number(v); });
}

QList<int> intListFromStringList(const QStringList &list)
{
    return Utils::transform<QList<int> >(list, [](const QString &v) { return v.toInt(); });
}

static QStringList defaultDisabledMessagesAsString()
{
    static const QStringList result = intListToStringList(defaultDisabledMessages());
    return result;
}

static QStringList defaultDisabledNonQuickUiAsString()
{
    static const QStringList result = intListToStringList(defaultDisabledMessagesNonQuickUi());
    return result;
}

void QmlJsEditingSettings::fromSettings(QtcSettings *settings)
{
    settings->beginGroup(QmlJSEditor::Constants::SETTINGS_CATEGORY_QML);
    m_enableContextPane = settings->value(QML_CONTEXTPANE_KEY, QVariant(false)).toBool();
    m_pinContextPane = settings->value(QML_CONTEXTPANEPIN_KEY, QVariant(false)).toBool();
    m_autoFormatOnSave = settings->value(AUTO_FORMAT_ON_SAVE, QVariant(false)).toBool();
    m_autoFormatOnlyCurrentProject
        = settings->value(AUTO_FORMAT_ONLY_CURRENT_PROJECT, QVariant(false)).toBool();
    m_foldAuxData = settings->value(FOLD_AUX_DATA, QVariant(true)).toBool();
    m_uiQmlOpenMode = settings->value(UIQML_OPEN_MODE, "").toString();
    m_qmllsSettings.useQmlls = settings->value(USE_QMLLS, QVariant(false)).toBool();
    m_qmllsSettings.useLatestQmlls = settings->value(USE_LATEST_QMLLS, QVariant(false)).toBool();
    m_qmllsSettings.disableBuiltinCodemodel
        = settings->value(DISABLE_BUILTIN_CODEMODEL, QVariant(false)).toBool();
    m_formatCommand = settings->value(FORMAT_COMMAND, {}).toString();
    m_formatCommandOptions = settings->value(FORMAT_COMMAND_OPTIONS, {}).toString();
    m_useCustomFormatCommand = settings->value(CUSTOM_COMMAND, QVariant(false)).toBool();
    m_useCustomAnalyzer = settings->value(CUSTOM_ANALYZER, QVariant(false)).toBool();

    m_disabledMessages = Utils::toSet(
        intListFromStringList(settings->value(DISABLED_MESSAGES,
                              defaultDisabledMessagesAsString()).toStringList()));

    m_disabledMessagesForNonQuickUi = Utils::toSet(
        intListFromStringList(settings->value(DISABLED_MESSAGES_NONQUICKUI,
                              defaultDisabledNonQuickUiAsString()).toStringList()));

    settings->endGroup();
}

void QmlJsEditingSettings::toSettings(QtcSettings *settings) const
{
    settings->beginGroup(QmlJSEditor::Constants::SETTINGS_CATEGORY_QML);
    settings->setValue(QML_CONTEXTPANE_KEY, m_enableContextPane);
    settings->setValue(QML_CONTEXTPANEPIN_KEY, m_pinContextPane);
    settings->setValue(AUTO_FORMAT_ON_SAVE, m_autoFormatOnSave);
    settings->setValue(AUTO_FORMAT_ONLY_CURRENT_PROJECT, m_autoFormatOnlyCurrentProject);
    settings->setValue(FOLD_AUX_DATA, m_foldAuxData);
    settings->setValue(UIQML_OPEN_MODE, m_uiQmlOpenMode);
    settings->setValue(USE_QMLLS, m_qmllsSettings.useQmlls);
    settings->setValue(USE_LATEST_QMLLS, m_qmllsSettings.useLatestQmlls);
    settings->setValue(DISABLE_BUILTIN_CODEMODEL, m_qmllsSettings.disableBuiltinCodemodel);
    settings->setValueWithDefault(FORMAT_COMMAND, m_formatCommand, {});
    settings->setValueWithDefault(FORMAT_COMMAND_OPTIONS, m_formatCommandOptions, {});
    settings->setValueWithDefault(CUSTOM_COMMAND, m_useCustomFormatCommand, false);
    settings->setValueWithDefault(CUSTOM_ANALYZER, m_useCustomAnalyzer, false);
    settings->setValueWithDefault(DISABLED_MESSAGES,
                                  intListToStringList(Utils::sorted(Utils::toList(m_disabledMessages))),
                                  defaultDisabledMessagesAsString());
    settings->setValueWithDefault(DISABLED_MESSAGES_NONQUICKUI,
                                  intListToStringList(Utils::sorted(Utils::toList(m_disabledMessagesForNonQuickUi))),
                                  defaultDisabledNonQuickUiAsString());
    settings->endGroup();
    QmllsSettingsManager::instance()->checkForChanges();
}

bool QmlJsEditingSettings::equals(const QmlJsEditingSettings &other) const
{
    return m_enableContextPane == other.m_enableContextPane
           && m_pinContextPane == other.m_pinContextPane
           && m_autoFormatOnSave == other.m_autoFormatOnSave
           && m_autoFormatOnlyCurrentProject == other.m_autoFormatOnlyCurrentProject
           && m_foldAuxData == other.m_foldAuxData && m_qmllsSettings == other.m_qmllsSettings
           && m_uiQmlOpenMode == other.m_uiQmlOpenMode && m_formatCommand == other.m_formatCommand
           && m_formatCommandOptions == other.m_formatCommandOptions
           && m_useCustomFormatCommand == other.m_useCustomFormatCommand
           && m_useCustomAnalyzer == other.m_useCustomAnalyzer
           && m_disabledMessages == other.m_disabledMessages
           && m_disabledMessagesForNonQuickUi == other.m_disabledMessagesForNonQuickUi;
}

bool QmlJsEditingSettings::enableContextPane() const
{
    return m_enableContextPane;
}

void QmlJsEditingSettings::setEnableContextPane(const bool enableContextPane)
{
    m_enableContextPane = enableContextPane;
}

bool QmlJsEditingSettings::pinContextPane() const
{
    return m_pinContextPane;
}

void QmlJsEditingSettings::setPinContextPane(const bool pinContextPane)
{
    m_pinContextPane = pinContextPane;
}

bool QmlJsEditingSettings::autoFormatOnSave() const
{
    return m_autoFormatOnSave;
}

void QmlJsEditingSettings::setAutoFormatOnSave(const bool autoFormatOnSave)
{
    m_autoFormatOnSave = autoFormatOnSave;
}

bool QmlJsEditingSettings::autoFormatOnlyCurrentProject() const
{
    return m_autoFormatOnlyCurrentProject;
}

void QmlJsEditingSettings::setAutoFormatOnlyCurrentProject(const bool autoFormatOnlyCurrentProject)
{
    m_autoFormatOnlyCurrentProject = autoFormatOnlyCurrentProject;
}

bool QmlJsEditingSettings::foldAuxData() const
{
    return m_foldAuxData;
}

void QmlJsEditingSettings::setFoldAuxData(const bool foldAuxData)
{
    m_foldAuxData = foldAuxData;
}

QString QmlJsEditingSettings::defaultFormatCommand() const
{
    return DEFAULT_CUSTOM_FORMAT_COMMAND;
}

QString QmlJsEditingSettings::formatCommand() const
{
    return m_formatCommand;
}

void QmlJsEditingSettings::setFormatCommand(const QString &formatCommand)
{
    m_formatCommand = formatCommand;
}

QString QmlJsEditingSettings::formatCommandOptions() const
{
    return m_formatCommandOptions;
}

void QmlJsEditingSettings::setFormatCommandOptions(const QString &formatCommandOptions)
{
    m_formatCommandOptions = formatCommandOptions;
}

bool QmlJsEditingSettings::useCustomFormatCommand() const
{
    return m_useCustomFormatCommand;
}

void QmlJsEditingSettings::setUseCustomFormatCommand(bool customCommand)
{
    m_useCustomFormatCommand = customCommand;
}

QmllsSettings &QmlJsEditingSettings::qmllsSettings()
{
    return m_qmllsSettings;
}

const QmllsSettings &QmlJsEditingSettings::qmllsSettings() const
{
    return m_qmllsSettings;
}

const QString QmlJsEditingSettings::uiQmlOpenMode() const
{
    return m_uiQmlOpenMode;
}

void QmlJsEditingSettings::setUiQmlOpenMode(const QString &mode)
{
    m_uiQmlOpenMode = mode;
}

bool QmlJsEditingSettings::useCustomAnalyzer() const
{
    return m_useCustomAnalyzer;
}

void QmlJsEditingSettings::setUseCustomAnalyzer(bool customAnalyzer)
{
    m_useCustomAnalyzer = customAnalyzer;
}

QSet<int> QmlJsEditingSettings::disabledMessages() const
{
    return m_disabledMessages;
}

void QmlJsEditingSettings::setDisabledMessages(const QSet<int> &disabled)
{
    m_disabledMessages = disabled;
}

QSet<int> QmlJsEditingSettings::disabledMessagesForNonQuickUi() const
{
    return m_disabledMessagesForNonQuickUi;
}

void QmlJsEditingSettings::setDisabledMessagesForNonQuickUi(const QSet<int> &disabled)
{
    m_disabledMessagesForNonQuickUi = disabled;
}

class AnalyzerMessageItem final : public Utils::TreeItem
{
public:
    AnalyzerMessageItem() = default;
    AnalyzerMessageItem(int number, const QString &message)
        : m_messageNumber(number)
        , m_message(message)
    {}

    QVariant data(int column, int role) const final
    {
        if (role == Qt::DisplayRole) {
            if (column == 0)
                return QString("M%1").arg(m_messageNumber);
            if (column == 2)
                return m_message.split('\n').first();
        } else if (role == Qt::CheckStateRole) {
            if (column == 0)
                return m_checked ? Qt::Checked : Qt::Unchecked;
            if (column == 1)
                return m_disabledInNonQuickUi ? Qt::Checked : Qt::Unchecked;
        }
        return TreeItem::data(column, role);
    }

    bool setData(int column, const QVariant &value, int role) final
    {
        if (role == Qt::CheckStateRole) {
            if (column == 0) {
                m_checked = value.toBool();
                return true;
            }
            if (column == 1) {
                m_disabledInNonQuickUi = value.toBool();
                return true;
            }
        }
        return false;
    }

    Qt::ItemFlags flags(int column) const final
    {
        if (column == 0 || column == 1)
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
        else
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }

    int messageNumber() const { return m_messageNumber; }
private:
    int m_messageNumber = -1;
    QString m_message;
    bool m_checked = true;
    bool m_disabledInNonQuickUi = false;
};

class QmlJsEditingSettingsPageWidget final : public Core::IOptionsPageWidget
{
public:
    QmlJsEditingSettingsPageWidget()
    {
        auto s = QmlJsEditingSettings::get();
        autoFormatOnSave = new QCheckBox(Tr::tr("Enable auto format on file save"));
        autoFormatOnSave->setChecked(s.autoFormatOnSave());
        autoFormatOnlyCurrentProject =
                new QCheckBox(Tr::tr("Restrict to files contained in the current project"));
        autoFormatOnlyCurrentProject->setChecked(s.autoFormatOnlyCurrentProject());
        autoFormatOnlyCurrentProject->setEnabled(autoFormatOnSave->isChecked());
        useCustomFormatCommand = new QCheckBox(
            Tr::tr("Use custom command instead of built-in formatter"));
        useCustomFormatCommand->setChecked(s.useCustomFormatCommand());
        auto formatCommandLabel = new QLabel(Tr::tr("Command:"));
        formatCommand = new QLineEdit();
        formatCommand->setText(s.formatCommand());
        formatCommand->setPlaceholderText(s.defaultFormatCommand());
        auto formatCommandOptionsLabel = new QLabel(Tr::tr("Arguments:"));
        formatCommandOptions = new QLineEdit();
        formatCommandOptions->setText(s.formatCommandOptions());
        pinContextPane = new QCheckBox(Tr::tr("Pin Qt Quick Toolbar"));
        pinContextPane->setChecked(s.pinContextPane());
        enableContextPane = new QCheckBox(Tr::tr("Always show Qt Quick Toolbar"));
        enableContextPane->setChecked(s.enableContextPane());
        foldAuxData = new QCheckBox(Tr::tr("Auto-fold auxiliary data"));
        foldAuxData->setChecked(s.foldAuxData());
        uiQmlOpenComboBox = new QComboBox;
        uiQmlOpenComboBox->addItem(Tr::tr("Always Ask"), "");
        uiQmlOpenComboBox->addItem(Tr::tr("Qt Design Studio"), Core::Constants::MODE_DESIGN);
        uiQmlOpenComboBox->addItem(Tr::tr("Qt Creator"), Core::Constants::MODE_EDIT);
        const int comboIndex = qMax(0, uiQmlOpenComboBox->findData(s.uiQmlOpenMode()));
        uiQmlOpenComboBox->setCurrentIndex(comboIndex);
        uiQmlOpenComboBox->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        uiQmlOpenComboBox->setSizeAdjustPolicy(QComboBox::QComboBox::AdjustToContents);

        useQmlls = new QCheckBox(Tr::tr("Enable QML Language Server (EXPERIMENTAL!)"));
        useQmlls->setChecked(s.qmllsSettings().useQmlls);
        disableBuiltInCodemodel = new QCheckBox(
            Tr::tr("Use QML Language Server advanced features (renaming, find usages and co.) "
                   "(EXPERIMENTAL!)"));
        disableBuiltInCodemodel->setChecked(s.qmllsSettings().disableBuiltinCodemodel);
        disableBuiltInCodemodel->setEnabled(s.qmllsSettings().useQmlls);
        useLatestQmlls = new QCheckBox(Tr::tr("Use QML Language Server from latest Qt version"));
        useLatestQmlls->setChecked(s.qmllsSettings().useLatestQmlls);
        useLatestQmlls->setEnabled(s.qmllsSettings().useQmlls);
        QObject::connect(useQmlls, &QCheckBox::stateChanged, this, [this](int checked) {
            useLatestQmlls->setEnabled(checked != Qt::Unchecked);
            disableBuiltInCodemodel->setEnabled(checked != Qt::Unchecked);
        });

        useCustomAnalyzer = new QCheckBox(Tr::tr("Use customized static analyzer"));
        useCustomAnalyzer->setChecked(s.useCustomAnalyzer());
        analyzerMessageModel = new Utils::TreeModel<AnalyzerMessageItem>(this);
        analyzerMessageModel->setHeader({Tr::tr("Enabled"),
                                         Tr::tr("Disabled for non Qt Quick UI"),
                                         Tr::tr("Message")});
        analyzerMessagesView = new QTreeView;
        analyzerMessagesView->setModel(analyzerMessageModel);
        analyzerMessagesView->setEnabled(s.useCustomAnalyzer());
        QObject::connect(useCustomAnalyzer, &QCheckBox::stateChanged, this, [this](int checked){
            analyzerMessagesView->setEnabled(checked != Qt::Unchecked);
        });
        analyzerMessagesView->setToolTip(
            "<html>"
            + Tr::tr("Enabled checks can be disabled for non Qt Quick UI"
                     " files, but disabled checks cannot get explicitly"
                     " enabled for non Qt Quick UI files."));
        analyzerMessagesView->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(analyzerMessagesView, &QTreeView::customContextMenuRequested,
                this, &QmlJsEditingSettingsPageWidget::showContextMenu);
        using namespace Layouting;
        // clang-format off
        QWidget *formattingGroup = nullptr;
        Column {
            Group {
                bindTo(&formattingGroup),
                title(Tr::tr("Automatic Formatting on File Save")),
                Column {
                    autoFormatOnSave,
                    autoFormatOnlyCurrentProject,
                    useCustomFormatCommand,
                    Form {
                        formatCommandLabel, formatCommand, br,
                        formatCommandOptionsLabel, formatCommandOptions
                    }
                },
            },
            Group {
                title(Tr::tr("Qt Quick Toolbars")),
                Column { pinContextPane, enableContextPane },
            },
            Group {
                title(Tr::tr("Features")),
                Column {
                    foldAuxData,
                    Form { Tr::tr("Open .ui.qml files with:"), uiQmlOpenComboBox },
                },
            },
            Group{
                title(Tr::tr("QML Language Server")),
                Column{useQmlls, disableBuiltInCodemodel , useLatestQmlls},
            },
            Group {
                title(Tr::tr("Static Analyzer")),
                Column{ useCustomAnalyzer, analyzerMessagesView },
            },
            st,
        }.attachTo(this);
        // clang-format on

        Utils::VariableChooser::addSupportForChildWidgets(formattingGroup,
                                                          Utils::globalMacroExpander());

        const auto updateFormatCommandState = [&, formatCommandLabel, formatCommandOptionsLabel] {
            const bool enabled = useCustomFormatCommand->isChecked();
            formatCommandLabel->setEnabled(enabled);
            formatCommand->setEnabled(enabled);
            formatCommandOptionsLabel->setEnabled(enabled);
            formatCommandOptions->setEnabled(enabled);
        };
        updateFormatCommandState();

        connect(autoFormatOnSave, &QCheckBox::toggled, this, [&, updateFormatCommandState]() {
            autoFormatOnlyCurrentProject->setEnabled(autoFormatOnSave->isChecked());
            updateFormatCommandState();
        });
        connect(useCustomFormatCommand, &QCheckBox::toggled, this, updateFormatCommandState);

        populateAnalyzerMessages(s.disabledMessages(), s.disabledMessagesForNonQuickUi());
    }

    void apply() final
    {
        QmlJsEditingSettings s;
        s.setEnableContextPane(enableContextPane->isChecked());
        s.setPinContextPane(pinContextPane->isChecked());
        s.setAutoFormatOnSave(autoFormatOnSave->isChecked());
        s.setAutoFormatOnlyCurrentProject(autoFormatOnlyCurrentProject->isChecked());
        s.setUseCustomFormatCommand(useCustomFormatCommand->isChecked());
        s.setFormatCommand(formatCommand->text());
        s.setFormatCommandOptions(formatCommandOptions->text());
        s.setFoldAuxData(foldAuxData->isChecked());
        s.setUiQmlOpenMode(uiQmlOpenComboBox->currentData().toString());
        s.qmllsSettings().useQmlls = useQmlls->isChecked();
        s.qmllsSettings().disableBuiltinCodemodel = disableBuiltInCodemodel->isChecked();
        s.qmllsSettings().useLatestQmlls = useLatestQmlls->isChecked();
        s.setUseCustomAnalyzer(useCustomAnalyzer->isChecked());
        QSet<int> disabled;
        QSet<int> disabledForNonQuickUi;
        analyzerMessageModel->forAllItems(
            [&disabled, &disabledForNonQuickUi](AnalyzerMessageItem *item){
            if (item->data(0, Qt::CheckStateRole) == Qt::Unchecked)
                disabled.insert(item->messageNumber());
            if (item->data(1, Qt::CheckStateRole) == Qt::Checked)
                disabledForNonQuickUi.insert(item->messageNumber());
        });
        s.setDisabledMessages(disabled);
        s.setDisabledMessagesForNonQuickUi(disabledForNonQuickUi);
        s.set();
    }

private:
    void populateAnalyzerMessages(const QSet<int> &disabled, const QSet<int> &disabledForNonQuickUi)
    {
        using namespace QmlJS::StaticAnalysis;
        auto knownMessages = Utils::sorted(Message::allMessageTypes());
        auto root = analyzerMessageModel->rootItem();
        for (auto msgType : knownMessages) {
            const QString msg = Message::prototypeForMessageType(msgType).message;
            auto item = new AnalyzerMessageItem(msgType, msg);
            item->setData(0, !disabled.contains(msgType), Qt::CheckStateRole);
            item->setData(1, disabledForNonQuickUi.contains(msgType), Qt::CheckStateRole);
            root->appendChild(item);
        }

        for (int column = 0; column < 3; ++column)
            analyzerMessagesView->resizeColumnToContents(column);
    }

    void showContextMenu(const QPoint &position)
    {
        QMenu menu;
        QAction *reset = new QAction(Tr::tr("Reset to Default"), &menu);
        menu.addAction(reset);
        connect(reset, &QAction::triggered, this, [this](){
            analyzerMessageModel->clear();
            populateAnalyzerMessages(Utils::toSet(defaultDisabledMessages()),
                                     Utils::toSet(defaultDisabledMessagesNonQuickUi()));

        });
        menu.exec(analyzerMessagesView->mapToGlobal(position));
    }

    QCheckBox *autoFormatOnSave;
    QCheckBox *autoFormatOnlyCurrentProject;
    QCheckBox *useCustomFormatCommand;
    QLineEdit *formatCommand;
    QLineEdit *formatCommandOptions;
    QCheckBox *pinContextPane;
    QCheckBox *enableContextPane;
    QCheckBox *foldAuxData;
    QCheckBox *useQmlls;
    QCheckBox *useLatestQmlls;
    QCheckBox *disableBuiltInCodemodel;
    QComboBox *uiQmlOpenComboBox;
    QCheckBox *useCustomAnalyzer;
    QTreeView *analyzerMessagesView;
    Utils::TreeModel<AnalyzerMessageItem> *analyzerMessageModel;
};


QmlJsEditingSettings QmlJsEditingSettings::get()
{
    QmlJsEditingSettings settings;
    settings.fromSettings(Core::ICore::settings());
    return settings;
}

QmlJsEditingSettingsPage::QmlJsEditingSettingsPage()
{
    setId("C.QmlJsEditing");
    setDisplayName(::QmlJSEditor::Tr::tr("QML/JS Editing"));
    setCategory(Constants::SETTINGS_CATEGORY_QML);
    setWidgetCreator([] { return new QmlJsEditingSettingsPageWidget; });
}

} // QmlJsEditor
