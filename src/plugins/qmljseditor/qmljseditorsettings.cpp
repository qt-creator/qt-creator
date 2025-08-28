// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljseditorsettings.h"
#include "qmljseditorconstants.h"
#include "qmljseditortr.h"
#include "qmllsclient.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>

#include <languageclient/languageclientinterface.h>
#include <languageclient/languageclientmanager.h>
#include <languageclient/languageclientsettings.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectpanelfactory.h>
#include <projectexplorer/projectsettingswidget.h>
#include <projectexplorer/projecttree.h>

#include <qmljs/qmljscheck.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/qmljsstaticanalysismessage.h>

#include <qmljstools/qmljstoolsconstants.h>

#include <qtsupport/qtsupportconstants.h>

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
#include <QPushButton>
#include <QTextStream>
#include <QTreeView>

#include <nanotrace/nanotrace.h>

#include <limits>

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

    disabledMessages.setSettingsKey(group, DISABLED_MESSAGES);
    disabledMessages.setDefaultValue(defaultDisabledMessages());
    disabledMessages.setFromSettingsTransformation(&fromSettingsTransformation);
    disabledMessages.setToSettingsTransformation(&toSettingsTransformation);

    disabledMessagesForNonQuickUi.setSettingsKey(group, DISABLED_MESSAGES_NONQUICKUI);
    disabledMessagesForNonQuickUi.setDefaultValue(defaultDisabledMessagesNonQuickUi());
    disabledMessagesForNonQuickUi.setFromSettingsTransformation(&fromSettingsTransformation);
    disabledMessagesForNonQuickUi.setToSettingsTransformation(&toSettingsTransformation);

    qdsCommand.setSettingsKey(group, QDS_COMMAND);
    qdsCommand.setPlaceHolderText(defaultQdsCommand().toUserOutput());
    qdsCommand.setLabelText(Tr::tr("Command:"));
    qdsCommand.setVisible(false);

    readSettings();
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
    Core::ICore::showOptionsDialog(QtSupport::Constants::QTVERSION_SETTINGS_PAGE_ID);
}

class QmlJsEditingSettingsPageWidget final : public Core::IOptionsPageWidget
{
public:
    QmlJsEditingSettingsPageWidget()
    {
        QmlJsEditingSettings &s = settings();

        analyzerMessageModel.setHeader(
            {Tr::tr("Enabled"), Tr::tr("Only for Qt Quick UI"), Tr::tr("Message")});
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
        QWidget *qdsGroup = nullptr;
        Column {
            Group {
                bindTo(&formattingGroup),
                title(Tr::tr("Formatting")),
                Column {
                    s.autoFormatOnSave,
                    s.autoFormatOnlyCurrentProject,
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
                bindTo(&qdsGroup),
                title(Tr::tr("Qt Design Studio")),
                Column {
                    Label {
                        wordWrap(true),
                        text(Tr::tr("Set the path to the Qt Design Studio application to enable "
                                    "the \"Open in Qt Design Studio\" feature. If you have Qt "
                                    "Design Studio installed alongside Qt Creator with the Qt "
                                    "Online Installer, it is used as the default. Use "
                                    "<a href=\"linwithqt\">\"Link with Qt\"</a> to link an "
                                    "offline installation of Qt Creator to a Qt Online Installer.")),
                        onLinkActivated(this, [](const QString &) { openQtVersionsOptions(); })
                    },
                    Form {
                        s.qdsCommand
                    }
                }
            },
            Group {
                title(Tr::tr("Features")),
                Column {
                    s.foldAuxData,
                    Row { s.uiQmlOpenMode, st }
                },
            },
            Group {
                title(Tr::tr("QML Language Server")),
                Row {
                    PushButton {
                        text(Tr::tr("Open Language Server preferences...")),
                        onClicked(this, [] { Core::ICore::showOptionsDialog(LanguageClient::Constants::LANGUAGECLIENT_SETTINGS_PAGE); })
                    },
                    st
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

        qdsGroup->setVisible(s.qdsCommand.isVisible());

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
    }

private:
    void populateAnalyzerMessages(const QList<int> &disabled, const QList<int> &disabledForNonQuickUi)
    {
        using namespace QmlJS::StaticAnalysis;
        const QList<Type> knownMessages = Utils::sorted(Message::allMessageTypes());
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
    setId(SETTINGS_PAGE);
    setDisplayName(::QmlJSEditor::Tr::tr("QML/JS Editing"));
    setCategory(Constants::SETTINGS_CATEGORY_QML);
    setWidgetCreator([] { return new QmlJsEditingSettingsPageWidget; });
    setSettingsProvider([] { return &settings(); });
}

class QmlJsEditingProjectSettingsWidget final : public ProjectSettingsWidget
{
public:
    explicit QmlJsEditingProjectSettingsWidget(Project *)
    {
        setUseGlobalSettingsCheckBoxVisible(false);
        setGlobalSettingsId(SETTINGS_PAGE);
        setExpanding(true);

        using namespace Layouting;
        // clang-format off
        Column {
            Group {
                title(Tr::tr("QML Language Server")),
                Row {
                    PushButton {
                        text(Tr::tr("Open Language Server preferences...")),
                        onClicked(this, [] { ProjectExplorerPlugin::activateProjectPanel(LanguageClient::Constants::LANGUAGECLIENT_SETTINGS_PANEL); })
                    },
                    st,
                },
            },
            tight,
            st,
        }.attachTo(this);
        // clang-format on
    }
};

class QmlJsEditingProjectPanelFactory : public ProjectPanelFactory
{
public:
    QmlJsEditingProjectPanelFactory()
    {
        setPriority(34); // just before the LanguageClientProjectPanelFactory
        setDisplayName(Tr::tr("Qt Quick"));
        setCreateWidgetFunction([](Project *project) {
            return new QmlJsEditingProjectSettingsWidget(project);
        });
    }
};

void setupQmlJsEditingProjectPanel()
{
    static QmlJsEditingProjectPanelFactory theQmlJsEditingProjectPanelFactory;
}


} // QmlJsEditor::Internal
