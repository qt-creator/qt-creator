// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljseditorsettings.h"
#include "qmljseditorconstants.h"
#include "qmljseditortr.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>

#include <qmljs/qmljscheck.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/qmljsstaticanalysismessage.h>

#include <qmljstools/qmljstoolsconstants.h>

#include <qtsupport/qtversionmanager.h>

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/macroexpander.h>
#include <utils/qtcsettings.h>
#include <utils/treemodel.h>
#include <utils/variablechooser.h>

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QLoggingCategory>
#include <QMenu>
#include <QTextStream>
#include <QTreeView>

#include <nanotrace/nanotrace.h>

#include <limits>

using namespace QmlJSEditor::Internal;
using namespace QtSupport;
using namespace Utils;

namespace QmlJSEditor::Internal {

static Q_LOGGING_CATEGORY(qmllsLog, "qtc.qmlls.settings", QtWarningMsg)

const char AUTO_FORMAT_ON_SAVE[] = "QmlJSEditor.AutoFormatOnSave";
const char AUTO_FORMAT_ONLY_CURRENT_PROJECT[] = "QmlJSEditor.AutoFormatOnlyCurrentProject";
const char QML_CONTEXTPANE_KEY[] = "QmlJSEditor.ContextPaneEnabled";
const char QML_CONTEXTPANEPIN_KEY[] = "QmlJSEditor.ContextPanePinned";
const char FOLD_AUX_DATA[] = "QmlJSEditor.FoldAuxData";
const char USE_QMLLS[] = "QmlJSEditor.UseQmlls";
const char USE_LATEST_QMLLS[] = "QmlJSEditor.UseLatestQmlls";
const char IGNORE_MINIMUM_QMLLS_VERSION[] = "QmlJSEditor.IgnoreMinimumQmllsVersion";
const char DISABLE_BUILTIN_CODEMODEL[] = "QmlJSEditor.DisableBuiltinCodemodel";
const char GENERATE_QMLLS_INI_FILES[] = "QmlJSEditor.GenerateQmllsIniFiles";
const char UIQML_OPEN_MODE[] = "QmlJSEditor.openUiQmlMode";
const char FORMAT_COMMAND[] = "QmlJSEditor.formatCommand";
const char FORMAT_COMMAND_OPTIONS[] = "QmlJSEditor.formatCommandOptions";
const char CUSTOM_COMMAND[] = "QmlJSEditor.useCustomFormatCommand";
const char CUSTOM_ANALYZER[] = "QmlJSEditor.useCustomAnalyzer";
const char DISABLED_MESSAGES[] = "QmlJSEditor.disabledMessages";
const char DISABLED_MESSAGES_NONQUICKUI[] = "QmlJSEditor.disabledMessagesNonQuickUI";
const char DEFAULT_CUSTOM_FORMAT_COMMAND[]
    = "%{CurrentDocument:Project:QT_HOST_BINS}/qmlformat%{HostOs:ExecutableSuffix}";

QmlJsEditingSettings &settings()
{
    static QmlJsEditingSettings settings;
    return settings;
}

static FilePath evaluateLatestQmlls()
{
    // find latest qmlls, i.e. vals
    if (!QtVersionManager::isLoaded())
        return {};
    const QtVersions versions = QtVersionManager::versions();
    FilePath latestQmlls;
    QVersionNumber latestVersion;
    FilePath latestQmakeFilePath;
    int latestUniqueId = std::numeric_limits<int>::min();
    for (QtVersion *v : versions) {
        // check if we find qmlls
        QVersionNumber vNow = v->qtVersion();
        FilePath qmllsNow = QmlJS::ModelManagerInterface::qmllsForBinPath(v->hostBinPath(), vNow);
        if (!qmllsNow.isExecutableFile())
            continue;
        if (latestVersion > vNow)
            continue;
        FilePath qmakeNow = v->qmakeFilePath();
        int uniqueIdNow = v->uniqueId();
        if (latestVersion == vNow) {
            if (latestQmakeFilePath > qmakeNow)
                continue;
            if (latestQmakeFilePath == qmakeNow && latestUniqueId >= v->uniqueId())
                continue;
        }
        latestVersion = vNow;
        latestQmlls = qmllsNow;
        latestQmakeFilePath = qmakeNow;
        latestUniqueId = uniqueIdNow;
    }
    return latestQmlls;
}

QmllsSettingsManager *QmllsSettingsManager::instance()
{
    static QmllsSettingsManager *manager = new QmllsSettingsManager;
    return manager;
}

FilePath QmllsSettingsManager::latestQmlls()
{
    QMutexLocker l(&m_mutex);
    return m_latestQmlls;
}

void QmllsSettingsManager::setupAutoupdate()
{
    QObject::connect(QtVersionManager::instance(),
                     &QtVersionManager::qtVersionsChanged,
                     this,
                     &QmllsSettingsManager::checkForChanges);
    if (QtVersionManager::isLoaded())
        checkForChanges();
    else
        QObject::connect(QtVersionManager::instance(),
                         &QtVersionManager::qtVersionsLoaded,
                         this,
                         &QmllsSettingsManager::checkForChanges);
}

void QmllsSettingsManager::checkForChanges()
{
    const QmlJsEditingSettings &newSettings = settings();
    FilePath newLatest = newSettings.useLatestQmlls() && newSettings.useQmlls()
            ? evaluateLatestQmlls() : m_latestQmlls;
    if (m_useQmlls == newSettings.useQmlls()
        && m_useLatestQmlls == newSettings.useLatestQmlls()
        && m_disableBuiltinCodemodel == newSettings.disableBuiltinCodemodel()
        && m_generateQmllsIniFiles == newSettings.generateQmllsIniFiles()
        && newLatest == m_latestQmlls)
        return;
    qCDebug(qmllsLog) << "qmlls settings changed:" << newSettings.useQmlls()
                      << newSettings.useLatestQmlls() << newLatest;
    {
        QMutexLocker l(&m_mutex);
        m_latestQmlls = newLatest;
        m_useQmlls = newSettings.useQmlls();
        m_useLatestQmlls = newSettings.useLatestQmlls();
        m_disableBuiltinCodemodel = newSettings.disableBuiltinCodemodel();
        m_generateQmllsIniFiles = newSettings.generateQmllsIniFiles();
    }
    emit settingsChanged();
}

bool QmllsSettingsManager::useLatestQmlls() const
{
    return m_useLatestQmlls;
}

bool QmllsSettingsManager::useQmlls() const
{
    return m_useQmlls;
}

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

static QVariant toSettingsTransformation(const QVariant &v)
{
    QList<int> list = v.value<QList<int>>();
    QStringList result = Utils::transform<QStringList>(list, [](int i) { return QString::number(i); });
    return QVariant::fromValue(result);
}

static QVariant fromSettingsTransformation(const QVariant &v)
{
    QStringList list = v.value<QStringList>();
    QList<int> result =  Utils::transform<QList<int>>(list, [](const QString &s) { return s.toInt(); });
    return QVariant::fromValue(result);
}

QmlJsEditingSettings::QmlJsEditingSettings()
{
    const Key group = QmlJSEditor::Constants::SETTINGS_CATEGORY_QML;

    useQmlls.setSettingsKey(group, USE_QMLLS);
    useQmlls.setDefaultValue(true);
    useQmlls.setLabelText(Tr::tr("Turn on"));

    enableContextPane.setSettingsKey(group, QML_CONTEXTPANE_KEY);
    enableContextPane.setLabelText(Tr::tr("Always show Qt Quick Toolbar"));

    pinContextPane.setSettingsKey(group, QML_CONTEXTPANEPIN_KEY);
    pinContextPane.setLabelText(Tr::tr("Pin Qt Quick Toolbar"));

    autoFormatOnSave.setSettingsKey(group, AUTO_FORMAT_ON_SAVE);
    autoFormatOnSave.setLabelText(Tr::tr("Enable auto format on file save"));

    autoFormatOnlyCurrentProject.setSettingsKey(group, AUTO_FORMAT_ONLY_CURRENT_PROJECT);
    autoFormatOnlyCurrentProject.setLabelText(
        Tr::tr("Restrict to files contained in the current project"));

    foldAuxData.setSettingsKey(group, FOLD_AUX_DATA);
    foldAuxData.setDefaultValue(true);
    foldAuxData.setLabelText(Tr::tr("Auto-fold auxiliary data"));

    uiQmlOpenMode.setSettingsKey(group, UIQML_OPEN_MODE);
    uiQmlOpenMode.setUseDataAsSavedValue();
    uiQmlOpenMode.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    uiQmlOpenMode.setLabelText(Tr::tr("Open .ui.qml files with:"));
    uiQmlOpenMode.addOption({Tr::tr("Always Ask")});
    uiQmlOpenMode.addOption({Tr::tr("Qt Design Studio"), {}, Core::Constants::MODE_DESIGN});
    uiQmlOpenMode.addOption({Tr::tr("Qt Creator"), {}, Core::Constants::MODE_EDIT});

    useLatestQmlls.setSettingsKey(group, USE_LATEST_QMLLS);
    useLatestQmlls.setLabelText(Tr::tr("Use from latest Qt version"));

    disableBuiltinCodemodel.setSettingsKey(group, DISABLE_BUILTIN_CODEMODEL);
    disableBuiltinCodemodel.setLabelText(
        Tr::tr("Use advanced features (renaming, find usages, and so on) "
               "(experimental)"));

    generateQmllsIniFiles.setSettingsKey(group, GENERATE_QMLLS_INI_FILES);
    generateQmllsIniFiles.setLabelText(
        Tr::tr("Create .qmlls.ini files for new projects"));

    ignoreMinimumQmllsVersion.setSettingsKey(group, IGNORE_MINIMUM_QMLLS_VERSION);
    ignoreMinimumQmllsVersion.setLabelText(
        Tr::tr("Allow versions below Qt %1")
            .arg(QmlJsEditingSettings::mininumQmllsVersion.toString()));

    useCustomFormatCommand.setSettingsKey(group, CUSTOM_COMMAND);
    useCustomFormatCommand.setLabelText(
        Tr::tr("Use custom command instead of built-in formatter"));

    formatCommand.setSettingsKey(group, FORMAT_COMMAND);
    formatCommand.setDisplayStyle(StringAspect::LineEditDisplay);
    formatCommand.setPlaceHolderText(defaultFormatCommand());
    formatCommand.setLabelText(Tr::tr("Command:"));

    formatCommandOptions.setSettingsKey(group, FORMAT_COMMAND_OPTIONS);
    formatCommandOptions.setDisplayStyle(StringAspect::LineEditDisplay);
    formatCommandOptions.setLabelText(Tr::tr("Arguments:"));

    useCustomAnalyzer.setSettingsKey(group, CUSTOM_ANALYZER);
    useCustomAnalyzer.setLabelText(Tr::tr("Use customized static analyzer"));

    disabledMessages.setSettingsKey(group, DISABLED_MESSAGES);
    disabledMessages.setDefaultValue(defaultDisabledMessages());
    disabledMessages.setFromSettingsTransformation(&fromSettingsTransformation);
    disabledMessages.setToSettingsTransformation(&toSettingsTransformation);

    disabledMessagesForNonQuickUi.setSettingsKey(group, DISABLED_MESSAGES_NONQUICKUI);
    disabledMessagesForNonQuickUi.setDefaultValue(defaultDisabledMessagesNonQuickUi());
    disabledMessagesForNonQuickUi.setFromSettingsTransformation(&fromSettingsTransformation);
    disabledMessagesForNonQuickUi.setToSettingsTransformation(&toSettingsTransformation);

    readSettings();

    autoFormatOnlyCurrentProject.setEnabler(&autoFormatOnSave);
    useLatestQmlls.setEnabler(&useQmlls);
    disableBuiltinCodemodel.setEnabler(&useQmlls);
    generateQmllsIniFiles.setEnabler(&useQmlls);
    ignoreMinimumQmllsVersion.setEnabler(&useQmlls);
    formatCommand.setEnabler(&useCustomFormatCommand);
    formatCommandOptions.setEnabler(&useCustomFormatCommand);
}

QString QmlJsEditingSettings::defaultFormatCommand() const
{
    return DEFAULT_CUSTOM_FORMAT_COMMAND;
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
        QmlJsEditingSettings &s = settings();

        analyzerMessageModel.setHeader({Tr::tr("Enabled"),
                                        Tr::tr("Disabled for non Qt Quick UI"),
                                        Tr::tr("Message")});
        analyzerMessagesView = new QTreeView;
        analyzerMessagesView->setModel(&analyzerMessageModel);
        analyzerMessagesView->setEnabled(s.useCustomAnalyzer());
        QObject::connect(&s.useCustomAnalyzer, &BoolAspect::volatileValueChanged, this, [this, &s] {
            analyzerMessagesView->setEnabled(s.useCustomAnalyzer.volatileValue());
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
                    s.autoFormatOnSave,
                    s.autoFormatOnlyCurrentProject,
                    s.useCustomFormatCommand,
                    Form {
                        s.formatCommand, br,
                        s.formatCommandOptions
                    }
                },
            },
            Group {
                title(Tr::tr("Qt Quick Toolbars")),
                Column {
                    s.pinContextPane,
                    s.enableContextPane
                },
            },
            Group {
                title(Tr::tr("Features")),
                Column {
                    s.foldAuxData,
                    Row { s.uiQmlOpenMode, st }
                },
            },
            Group{
                title(Tr::tr("QML Language Server")),
                Column {
                    s.useQmlls,
                    s.ignoreMinimumQmllsVersion,
                    s.disableBuiltinCodemodel,
                    s.useLatestQmlls,
                    s.generateQmllsIniFiles
                },
            },
            Group {
                title(Tr::tr("Static Analyzer")),
                Column {
                    s.useCustomAnalyzer,
                    analyzerMessagesView
                },
            },
            st,
        }.attachTo(this);
        // clang-format on

        Utils::VariableChooser::addSupportForChildWidgets(formattingGroup,
                                                          Utils::globalMacroExpander());

        populateAnalyzerMessages(s.disabledMessages(), s.disabledMessagesForNonQuickUi());
    }

    void apply() final
    {
        QmlJsEditingSettings &s = settings();
        s.apply();
        QList<int> disabled;
        QList<int> disabledForNonQuickUi;

        analyzerMessageModel.forAllItems(
            [&disabled, &disabledForNonQuickUi](AnalyzerMessageItem *item){
            if (item->data(0, Qt::CheckStateRole) == Qt::Unchecked)
                disabled.append(item->messageNumber());
            if (item->data(1, Qt::CheckStateRole) == Qt::Checked)
                disabledForNonQuickUi.append(item->messageNumber());
        });
        s.disabledMessages.setValue(disabled);
        s.disabledMessagesForNonQuickUi.setValue(disabledForNonQuickUi);
        s.writeSettings();
        QmllsSettingsManager::instance()->checkForChanges();
    }

private:
    void populateAnalyzerMessages(const QList<int> &disabled, const QList<int> &disabledForNonQuickUi)
    {
        using namespace QmlJS::StaticAnalysis;
        auto knownMessages = Utils::sorted(Message::allMessageTypes());
        auto root = analyzerMessageModel.rootItem();
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
            analyzerMessageModel.clear();
            populateAnalyzerMessages(defaultDisabledMessages(),
                                     defaultDisabledMessagesNonQuickUi());

        });
        menu.exec(analyzerMessagesView->mapToGlobal(position));
    }

    QTreeView *analyzerMessagesView;
    Utils::TreeModel<AnalyzerMessageItem> analyzerMessageModel;
};

QmlJsEditingSettingsPage::QmlJsEditingSettingsPage()
{
    setId("C.QmlJsEditing");
    setDisplayName(::QmlJSEditor::Tr::tr("QML/JS Editing"));
    setCategory(Constants::SETTINGS_CATEGORY_QML);
    setWidgetCreator([] { return new QmlJsEditingSettingsPageWidget; });
}

} // QmlJsEditor::Internal
