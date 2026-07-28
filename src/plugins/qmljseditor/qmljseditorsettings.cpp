// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljseditorsettings.h"
#include "qmljseditorconstants.h"
#include "qmljseditortr.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/dialogs/ioptionspage.h>

#include <languageclient/languageclientsettings.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projecttree.h>

#include <qmljs/qmljscheck.h>
#include <qmljs/qmljsstaticanalysismessage.h>

#include <qtsupport/qtsupportconstants.h>

#include <updateinfo/updateinfoservice.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/algorithm.h>
#include <utils/guiutils.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/qtcsettings.h>
#include <utils/treemodel.h>

#include <QGroupBox>
#include <QMenu>
#include <QPushButton>
#include <QTreeView>


using namespace QmlJSEditor::Internal;
using namespace QtSupport;
using namespace Utils;
using namespace ProjectExplorer;

namespace QmlJSEditor::Internal {

const char AUTO_FORMAT_ON_SAVE[] = "QmlJSEditor.AutoFormatOnSave";
const char AUTO_FORMAT_ONLY_CURRENT_PROJECT[] = "QmlJSEditor.AutoFormatOnlyCurrentProject";
const char QML_CONTEXTPANE_KEY[] = "QmlJSEditor.ContextPaneEnabled";
const char QML_CONTEXTPANEPIN_KEY[] = "QmlJSEditor.ContextPanePinned";
const char FOLD_AUX_DATA[] = "QmlJSEditor.FoldAuxData";
const char UIQML_OPEN_MODE[] = "QmlJSEditor.openUiQmlMode";
const char CUSTOM_ANALYZER[] = "QmlJSEditor.useCustomAnalyzer";
const char DISABLED_MESSAGES[] = "QmlJSEditor.disabledMessages";
const char DISABLED_MESSAGES_NONQUICKUI[] = "QmlJSEditor.disabledMessagesNonQuickUI";
const char QDS_COMMAND[] = "QmlJSEditor.qdsCommand";
const char SETTINGS_PAGE[] = "C.QmlJsEditing";

QmlJsEditingSettings &settings()
{
    static QmlJsEditingSettings settings;
    return settings;
}

using namespace LanguageClient;

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

static void openQtVersionsOptions();

QmlJsEditingSettings::QmlJsEditingSettings()
{
    setAutoApply(false);
    const Key group = QmlJSEditor::Constants::SETTINGS_CATEGORY_QML;

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

    useCustomAnalyzer.setSettingsKey(group, CUSTOM_ANALYZER);
    useCustomAnalyzer.setLabelText(Tr::tr("Use customized static analyzer"));

    qdsCommand.setSettingsKey(group, QDS_COMMAND);
    qdsCommand.setPlaceHolderText(defaultQdsCommand().toUserOutput());
    qdsCommand.setLabelText(Tr::tr("Command:"));
    qdsCommand.setVisible(false);

    setLayouter([this] {
        using namespace Layouting;
        // clang-format off
        Column column {
            Group {
                title(Tr::tr("Formatting")),
                Column {
                    autoFormatOnSave,
                    autoFormatOnlyCurrentProject,
                },
            },
            Group {
                title(Tr::tr("Qt Quick Toolbars")),
                Column {
                    pinContextPane,
                    enableContextPane
                },
            },
            Group {
                visibleOn(&qdsCommand),
                title(Tr::tr("Qt Design Studio")),
                Column {
                    Label {
                        wordWrap(true),
                        text(Tr::tr("Set the path to the Qt Design Studio application to enable "
                                    "the \"Open in Qt Design Studio\" feature. If you have Qt "
                                    "Design Studio installed alongside Qt Creator with the Qt "
                                    "Online Installer, it is used as the default. Use "
                                    "<a href=\"linkwithqt\">\"Link with Qt\"</a> to link an "
                                    "offline installation of Qt Creator to a Qt Online Installer.")),
                        onLinkActivated(this, [](const QString &) { openQtVersionsOptions(); })
                    },
                    Form {
                        qdsCommand, br
                    },
                    qdsInstall,
                },
            },
            Group {
                title(Tr::tr("Features")),
                Column {
                    foldAuxData,
                    Row { uiQmlOpenMode, st }
                },
            },
            Group {
                title(Tr::tr("QML Language Server")),
                Row {
                    PushButton {
                        text(Tr::tr("Open Language Server preferences...")),
                        onClicked(this, [] { Core::ICore::showSettings(LanguageClient::Constants::LANGUAGECLIENT_SETTINGS_PAGE); })
                    },
                    st
                },
            },
            Group {
                title(Tr::tr("Static Analyzer")),
                Column {
                    useCustomAnalyzer,
                    analyzerMessages
                },
            },
            st,
        };
        // clang-format on

        return column;
    });

    readSettings();

    analyzerMessages.setEnabler(&useCustomAnalyzer);
}

FilePath QmlJsEditingSettings::defaultQdsCommand() const
{
    QtcSettings *settings = Core::ICore::settings();
    const Key qdsInstallationEntry = "QML/Designer/DesignStudioInstallation"; //set in installer
    return FilePath::fromUserInput(settings->value(qdsInstallationEntry).toString());
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

static void openQtVersionsOptions()
{
    Core::ICore::showSettings(QtSupport::Constants::QTVERSION_SETTINGS_PAGE_ID);
}

static UpdateInfo::Service *updateInfoService()
{
    return ExtensionSystem::PluginManager::getObject<UpdateInfo::Service>();
}

static QStringList disabledMessagesToStringList(const QList<int> &list)
{
    return Utils::transform<QStringList>(list, [](int i) { return QString::number(i); });
}

static QList<int> disabledMessagesFromStringList(const QStringList &list)
{
    return Utils::transform<QList<int>>(list, [](const QString &s) { return s.toInt(); });
}

static void populateMessageModel(QTreeView *view, const QList<int> &disabled,
                                 const QList<int> &disabledForNonQuickUi)
{
    using namespace QmlJS::StaticAnalysis;
    auto model = static_cast<TreeModel<AnalyzerMessageItem> *>(view->model());
    model->clear();
    TreeItem *root = model->rootItem();
    const QList<Type> knownMessages = Utils::sorted(Message::allMessageTypes());
    for (Type msgType : knownMessages) {
        const QString msg = Message::prototypeForMessageType(msgType).message;
        auto item = new AnalyzerMessageItem(msgType, msg);
        item->setData(0, !disabled.contains(msgType), Qt::CheckStateRole);
        item->setData(1, disabledForNonQuickUi.contains(msgType), Qt::CheckStateRole);
        root->appendChild(item);
    }
    for (int column = 0; column < 3; ++column)
        view->resizeColumnToContents(column);
}

static void extractMessageModel(QTreeView *view, QList<int> &disabled,
                                QList<int> &disabledForNonQuickUi)
{
    auto model = static_cast<TreeModel<AnalyzerMessageItem> *>(view->model());
    model->forAllItems([&disabled, &disabledForNonQuickUi](AnalyzerMessageItem *item) {
        if (item->data(0, Qt::CheckStateRole) == Qt::Unchecked)
            disabled.append(item->messageNumber());
        if (item->data(1, Qt::CheckStateRole) == Qt::Checked)
            disabledForNonQuickUi.append(item->messageNumber());
    });
}

AnalyzerMessagesAspect::AnalyzerMessagesAspect(AspectContainer *container)
    : BaseAspect(container)
{}

void AnalyzerMessagesAspect::apply()
{
    if (!m_view)
        return;
    m_disabled.clear();
    m_disabledForNonQuickUi.clear();
    extractMessageModel(m_view, m_disabled, m_disabledForNonQuickUi);
}

void AnalyzerMessagesAspect::cancel()
{
    if (m_view)
        populateModel();
}

bool AnalyzerMessagesAspect::isDirty() const
{
    if (!m_view)
        return false;
    QList<int> disabled;
    QList<int> disabledForNonQuickUi;
    extractMessageModel(m_view, disabled, disabledForNonQuickUi);
    return Utils::sorted(disabled) != Utils::sorted(m_disabled)
           || Utils::sorted(disabledForNonQuickUi) != Utils::sorted(m_disabledForNonQuickUi);
}

void AnalyzerMessagesAspect::readSettings()
{
    QtcSettings &s = Utils::userSettings();
    s.beginGroup(QmlJSEditor::Constants::SETTINGS_CATEGORY_QML);
    m_disabled = disabledMessagesFromStringList(
        s.value(DISABLED_MESSAGES, disabledMessagesToStringList(defaultDisabledMessages()))
            .toStringList());
    m_disabledForNonQuickUi = disabledMessagesFromStringList(
        s.value(DISABLED_MESSAGES_NONQUICKUI,
                disabledMessagesToStringList(defaultDisabledMessagesNonQuickUi()))
            .toStringList());
    s.endGroup();
    if (m_view)
        populateModel();
}

void AnalyzerMessagesAspect::writeSettings() const
{
    QtcSettings &s = Utils::userSettings();
    s.beginGroup(QmlJSEditor::Constants::SETTINGS_CATEGORY_QML);
    s.setValue(DISABLED_MESSAGES, disabledMessagesToStringList(m_disabled));
    s.setValue(DISABLED_MESSAGES_NONQUICKUI, disabledMessagesToStringList(m_disabledForNonQuickUi));
    s.endGroup();
}

void AnalyzerMessagesAspect::populateModel()
{
    populateMessageModel(m_view, m_disabled, m_disabledForNonQuickUi);
}

void AnalyzerMessagesAspect::addToLayoutImpl(Layouting::Layout &parent)
{
    m_view = createSubWidget<QTreeView>();
    auto model = new TreeModel<AnalyzerMessageItem>(m_view);
    model->setHeader({Tr::tr("Enabled"), Tr::tr("Only for Qt Quick UI"), Tr::tr("Message")});
    m_view->setModel(model);
    m_view->setToolTip(
        "<html>"
        + Tr::tr("Enabled checks can be disabled for non Qt Quick UI"
                 " files, but disabled checks cannot get explicitly"
                 " enabled for non Qt Quick UI files."));
    m_view->setContextMenuPolicy(Qt::CustomContextMenu);

    populateModel();

    connect(model, &QAbstractItemModel::dataChanged, this, &checkSettingsDirty);

    connect(m_view, &QTreeView::customContextMenuRequested, this, [this](const QPoint &pos) {
        QMenu menu;
        QAction *reset = menu.addAction(Tr::tr("Reset to Default"));
        connect(reset, &QAction::triggered, this, [this] {
            populateMessageModel(m_view, defaultDisabledMessages(),
                                 defaultDisabledMessagesNonQuickUi());
            checkSettingsDirty();
        });
        menu.exec(m_view->mapToGlobal(pos));
    });

    parent.addItem(m_view);
}

QdsInstallAspect::QdsInstallAspect(AspectContainer *container)
    : BaseAspect(container)
{}

void QdsInstallAspect::addToLayoutImpl(Layouting::Layout &parent)
{
    using namespace Layouting;
    auto button = new QPushButton(Tr::tr("Install Qt Design Studio"));
    QWidget *row = Row { st, button, noMargin }.emerge();

    const auto update = [row] {
        QmlJsEditingSettings &s = settings();
        const QString placeholder = s.defaultQdsCommand().toUserOutput();
        s.qdsCommand.setPlaceHolderText(placeholder);
        if (PathChooser *chooser = s.qdsCommand.pathChooser())
            chooser->setPlaceholderText(placeholder);
        row->setVisible(s.defaultQdsCommand().isEmpty() && updateInfoService());
    };
    // Do not show the not-yet-parented row; it would briefly pop up as a window.
    if (!settings().defaultQdsCommand().isEmpty() || !updateInfoService())
        row->setVisible(false);

    connect(button, &QPushButton::clicked, this, [update] {
        UpdateInfo::Service *updater = updateInfoService();
        QTC_ASSERT(updater, return);
        if (updater->installPackages("^qt[.].*qtdesignstudio.*$")) {
            update();
            emit settings().qdsCommand.changed();
        }
    });

    parent.addItem(row);
}

class QmlJsEditingSettingsPage : public Core::IOptionsPage
{
public:
    QmlJsEditingSettingsPage()
    {
        setId(SETTINGS_PAGE);
        setDisplayName(::QmlJSEditor::Tr::tr("QML/JS Editing"));
        setCategory(Constants::SETTINGS_CATEGORY_QML);
        setSettingsProvider([] { return &settings(); });
    }
};

void setupQmlJsEditingSettings()
{
    static QmlJsEditingSettingsPage theQmlJsEditingSettingsPage;
}


} // QmlJsEditor::Internal
