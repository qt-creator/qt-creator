// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "editorconfiguration.h"

#include "project.h"
#include "projectexplorertr.h"
#include "projectmanager.h"

#include <utils/algorithm.h>

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>

#include <texteditor/texteditor.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/simplecodestylepreferences.h>
#include <texteditor/typingsettings.h>
#include <texteditor/storagesettings.h>
#include <texteditor/behaviorsettings.h>
#include <texteditor/extraencodingsettings.h>
#include <texteditor/tabsettings.h>
#include <texteditor/marginsettings.h>
#include <texteditor/icodestylepreferencesfactory.h>

#include <QTextCodec>
#include <QDebug>

using namespace TextEditor;
using namespace Utils;

namespace ProjectExplorer {

const Key kPrefix("EditorConfiguration.");
const Key kUseGlobal("EditorConfiguration.UseGlobal");
const Key kCodec("EditorConfiguration.Codec");
const Key kCodeStylePrefix("EditorConfiguration.CodeStyle.");
const Key kCodeStyleCount("EditorConfiguration.CodeStyle.Count");

struct EditorConfigurationPrivate
{
    EditorConfigurationPrivate() :
        m_typingSettings(TextEditorSettings::typingSettings()),
        m_storageSettings(TextEditorSettings::storageSettings()),
        m_behaviorSettings(TextEditorSettings::behaviorSettings()),
        m_extraEncodingSettings(TextEditorSettings::extraEncodingSettings()),
        m_textCodec(Core::EditorManager::defaultTextCodec())
    { }

    ICodeStylePreferences *m_defaultCodeStyle = nullptr;
    TypingSettings m_typingSettings;
    StorageSettings m_storageSettings;
    BehaviorSettings m_behaviorSettings;
    bool m_useGlobal = true;
    ExtraEncodingSettings m_extraEncodingSettings;
    MarginSettings m_marginSettings;
    QTextCodec *m_textCodec;

    QMap<Utils::Id, ICodeStylePreferences *> m_languageCodeStylePreferences;
    QList<BaseTextEditor *> m_editors;
};

EditorConfiguration::EditorConfiguration() : d(std::make_unique<EditorConfigurationPrivate>())
{
    const QMap<Utils::Id, ICodeStylePreferences *> languageCodeStylePreferences = TextEditorSettings::codeStyles();
    for (auto itCodeStyle = languageCodeStylePreferences.cbegin(), end = languageCodeStylePreferences.cend();
            itCodeStyle != end; ++itCodeStyle) {
        Utils::Id languageId = itCodeStyle.key();
        // global prefs for language
        ICodeStylePreferences *originalPreferences = itCodeStyle.value();
        ICodeStylePreferencesFactory *factory = TextEditorSettings::codeStyleFactory(languageId);
        // clone of global prefs for language - it will became project prefs for language
        ICodeStylePreferences *preferences = factory->createCodeStyle();
        // project prefs can point to the global language pool, which contains also the global language prefs
        preferences->setDelegatingPool(TextEditorSettings::codeStylePool(languageId));
        preferences->setId(languageId.name() + "Project");
        preferences->setDisplayName(Tr::tr("Project %1", "Settings, %1 is a language (C++ or QML)").arg(factory->displayName()));
        // project prefs by default point to global prefs (which in turn can delegate to anything else or not)
        preferences->setCurrentDelegate(originalPreferences);
        d->m_languageCodeStylePreferences.insert(languageId, preferences);
    }

    // clone of global prefs (not language specific), for project scope
    d->m_defaultCodeStyle = new SimpleCodeStylePreferences(this);
    d->m_defaultCodeStyle->setDelegatingPool(TextEditorSettings::codeStylePool());
    d->m_defaultCodeStyle->setDisplayName(Tr::tr("Project", "Settings"));
    d->m_defaultCodeStyle->setId("Project");
    // if setCurrentDelegate is 0 values are read from *this prefs
    d->m_defaultCodeStyle->setCurrentDelegate(TextEditorSettings::codeStyle());

    connect(ProjectManager::instance(), &ProjectManager::aboutToRemoveProject,
            this, &EditorConfiguration::slotAboutToRemoveProject);
}

EditorConfiguration::~EditorConfiguration()
{
    qDeleteAll(d->m_languageCodeStylePreferences);
}

bool EditorConfiguration::useGlobalSettings() const
{
    return d->m_useGlobal;
}

void EditorConfiguration::cloneGlobalSettings()
{
    d->m_defaultCodeStyle->setTabSettings(TextEditorSettings::codeStyle()->tabSettings());
    setTypingSettings(TextEditorSettings::typingSettings());
    setStorageSettings(TextEditorSettings::storageSettings());
    setBehaviorSettings(TextEditorSettings::behaviorSettings());
    setExtraEncodingSettings(TextEditorSettings::extraEncodingSettings());
    setMarginSettings(TextEditorSettings::marginSettings());
    d->m_textCodec = Core::EditorManager::defaultTextCodec();
}

QTextCodec *EditorConfiguration::textCodec() const
{
    return d->m_textCodec;
}

const TypingSettings &EditorConfiguration::typingSettings() const
{
    return d->m_typingSettings;
}

const StorageSettings &EditorConfiguration::storageSettings() const
{
    return d->m_storageSettings;
}

const BehaviorSettings &EditorConfiguration::behaviorSettings() const
{
    return d->m_behaviorSettings;
}

const ExtraEncodingSettings &EditorConfiguration::extraEncodingSettings() const
{
    return d->m_extraEncodingSettings;
}

const MarginSettings &EditorConfiguration::marginSettings() const
{
    return d->m_marginSettings;
}

ICodeStylePreferences *EditorConfiguration::codeStyle() const
{
    return d->m_defaultCodeStyle;
}

ICodeStylePreferences *EditorConfiguration::codeStyle(Utils::Id languageId) const
{
    return d->m_languageCodeStylePreferences.value(languageId, codeStyle());
}

QMap<Utils::Id, ICodeStylePreferences *> EditorConfiguration::codeStyles() const
{
    return d->m_languageCodeStylePreferences;
}

static void toMapWithPrefix(Store *map, const Store &source)
{
    for (auto it = source.constBegin(), end = source.constEnd(); it != end; ++it)
        map->insert(kPrefix + it.key(), it.value());
}

Store EditorConfiguration::toMap() const
{
    Store map = {
        {kUseGlobal, d->m_useGlobal},
        {kCodec, d->m_textCodec->name()},
        {kCodeStyleCount, d->m_languageCodeStylePreferences.count()}
    };

    int i = 0;
    for (auto itCodeStyle = d->m_languageCodeStylePreferences.cbegin(),
               end = d->m_languageCodeStylePreferences.cend();
            itCodeStyle != end; ++itCodeStyle) {
        const Store settingsIdMap = {
            {"language", QVariant::fromValue(itCodeStyle.key().toSetting())},
            {"value", QVariant::fromValue(itCodeStyle.value()->toMap())}
        };
        map.insert(numberedKey(kCodeStylePrefix, i), variantFromStore(settingsIdMap));
        i++;
    }

    toMapWithPrefix(&map, d->m_defaultCodeStyle->tabSettings().toMap());
    toMapWithPrefix(&map, d->m_typingSettings.toMap());
    toMapWithPrefix(&map, d->m_storageSettings.toMap());
    toMapWithPrefix(&map, d->m_behaviorSettings.toMap());
    toMapWithPrefix(&map, d->m_extraEncodingSettings.toMap());
    toMapWithPrefix(&map, d->m_marginSettings.toMap());

    return map;
}

void EditorConfiguration::fromMap(const Store &map)
{
    const QByteArray &codecName = map.value(kCodec, d->m_textCodec->name()).toByteArray();
    d->m_textCodec = QTextCodec::codecForName(codecName);
    if (!d->m_textCodec)
        d->m_textCodec = Core::EditorManager::defaultTextCodec();

    const int codeStyleCount = map.value(kCodeStyleCount, 0).toInt();
    for (int i = 0; i < codeStyleCount; ++i) {
        Store settingsIdMap = storeFromVariant(map.value(numberedKey(kCodeStylePrefix, i)));
        if (settingsIdMap.isEmpty()) {
            qWarning() << "No data for code style settings list" << i << "found!";
            continue;
        }
        Id languageId = Id::fromSetting(settingsIdMap.value("language"));
        Store value = storeFromVariant(settingsIdMap.value("value"));
        ICodeStylePreferences *preferences = d->m_languageCodeStylePreferences.value(languageId);
        if (preferences)
             preferences->fromMap(value);
    }

    Store submap;
    for (auto it = map.constBegin(), end = map.constEnd(); it != end; ++it) {
        if (it.key().view().startsWith(kPrefix.view()))
            submap.insert(it.key().toByteArray().mid(kPrefix.view().size()), it.value());
    }
    d->m_defaultCodeStyle->fromMap(submap);
    d->m_typingSettings.fromMap(submap);
    d->m_storageSettings.fromMap(submap);
    d->m_behaviorSettings.fromMap(submap);
    d->m_extraEncodingSettings.fromMap(submap);
    d->m_marginSettings.fromMap(submap);
    setUseGlobalSettings(map.value(kUseGlobal, d->m_useGlobal).toBool());
}

void EditorConfiguration::configureEditor(BaseTextEditor *textEditor) const
{
    TextEditorWidget *widget = textEditor->editorWidget();
    if (widget)
        widget->setCodeStyle(codeStyle(widget->languageSettingsId()));
    if (!d->m_useGlobal) {
        textEditor->textDocument()->setCodec(d->m_textCodec);
        if (widget)
            switchSettings(widget);
    }
    d->m_editors.append(textEditor);
    connect(textEditor, &BaseTextEditor::destroyed, this, [this, textEditor]() {
        d->m_editors.removeOne(textEditor);
    });
}

void EditorConfiguration::deconfigureEditor(BaseTextEditor *textEditor) const
{
    TextEditorWidget *widget = textEditor->editorWidget();
    if (widget)
        widget->setCodeStyle(TextEditorSettings::codeStyle(widget->languageSettingsId()));

    d->m_editors.removeOne(textEditor);

    // TODO: what about text codec and switching settings?
}

void EditorConfiguration::setUseGlobalSettings(bool use)
{
    d->m_useGlobal = use;
    d->m_defaultCodeStyle->setCurrentDelegate(use ? TextEditorSettings::codeStyle() : nullptr);
    const QList<Core::IEditor *> editors = Core::DocumentModel::editorsForOpenedDocuments();
    for (Core::IEditor *editor : editors) {
        if (auto widget = TextEditorWidget::fromEditor(editor)) {
            Project *project = ProjectManager::projectForFile(editor->document()->filePath());
            if (project && project->editorConfiguration() == this)
                switchSettings(widget);
        }
    }
}

template<typename New, typename Old>
static void switchSettings_helper(const New *newSender, const Old *oldSender,
                                  TextEditorWidget *widget)
{
    QObject::disconnect(oldSender, &Old::marginSettingsChanged,
                        widget, &TextEditorWidget::setMarginSettings);
    QObject::disconnect(oldSender, &Old::typingSettingsChanged,
                        widget, &TextEditorWidget::setTypingSettings);
    QObject::disconnect(oldSender, &Old::storageSettingsChanged,
                        widget, &TextEditorWidget::setStorageSettings);
    QObject::disconnect(oldSender, &Old::behaviorSettingsChanged,
                        widget, &TextEditorWidget::setBehaviorSettings);
    QObject::disconnect(oldSender, &Old::extraEncodingSettingsChanged,
                        widget, &TextEditorWidget::setExtraEncodingSettings);

    QObject::connect(newSender, &New::marginSettingsChanged,
                     widget, &TextEditorWidget::setMarginSettings);
    QObject::connect(newSender, &New::typingSettingsChanged,
                     widget, &TextEditorWidget::setTypingSettings);
    QObject::connect(newSender, &New::storageSettingsChanged,
                     widget, &TextEditorWidget::setStorageSettings);
    QObject::connect(newSender, &New::behaviorSettingsChanged,
                     widget, &TextEditorWidget::setBehaviorSettings);
    QObject::connect(newSender, &New::extraEncodingSettingsChanged,
                     widget, &TextEditorWidget::setExtraEncodingSettings);
}

void EditorConfiguration::switchSettings(TextEditorWidget *widget) const
{
    if (d->m_useGlobal) {
        widget->setMarginSettings(TextEditorSettings::marginSettings());
        widget->setTypingSettings(TextEditorSettings::typingSettings());
        widget->setStorageSettings(TextEditorSettings::storageSettings());
        widget->setBehaviorSettings(TextEditorSettings::behaviorSettings());
        widget->setExtraEncodingSettings(TextEditorSettings::extraEncodingSettings());
        switchSettings_helper(TextEditorSettings::instance(), this, widget);
    } else {
        widget->setMarginSettings(marginSettings());
        widget->setTypingSettings(typingSettings());
        widget->setStorageSettings(storageSettings());
        widget->setBehaviorSettings(behaviorSettings());
        widget->setExtraEncodingSettings(extraEncodingSettings());
        switchSettings_helper(this, TextEditorSettings::instance(), widget);
    }
}

void EditorConfiguration::setTypingSettings(const TypingSettings &settings)
{
    d->m_typingSettings = settings;
    emit typingSettingsChanged(d->m_typingSettings);
}

void EditorConfiguration::setStorageSettings(const StorageSettings &settings)
{
    d->m_storageSettings = settings;
    emit storageSettingsChanged(d->m_storageSettings);
}

void EditorConfiguration::setBehaviorSettings(const BehaviorSettings &settings)
{
    d->m_behaviorSettings = settings;
    emit behaviorSettingsChanged(d->m_behaviorSettings);
}

void EditorConfiguration::setExtraEncodingSettings(const ExtraEncodingSettings &settings)
{
    d->m_extraEncodingSettings = settings;
    emit extraEncodingSettingsChanged(d->m_extraEncodingSettings);
}

void EditorConfiguration::setMarginSettings(const MarginSettings &settings)
{
    if (d->m_marginSettings != settings) {
        d->m_marginSettings = settings;
        emit marginSettingsChanged(d->m_marginSettings);
    }
}

void EditorConfiguration::setTextCodec(QTextCodec *textCodec)
{
    d->m_textCodec = textCodec;
}

void EditorConfiguration::setShowWrapColumn(bool onoff)
{
    if (d->m_marginSettings.m_showMargin != onoff) {
        d->m_marginSettings.m_showMargin = onoff;
        emit marginSettingsChanged(d->m_marginSettings);
    }
}

void EditorConfiguration::setTintMarginArea(bool onoff)
{
    if (d->m_marginSettings.m_tintMarginArea != onoff) {
        d->m_marginSettings.m_tintMarginArea = onoff;
        emit marginSettingsChanged(d->m_marginSettings);
    }
}

void EditorConfiguration::setUseIndenter(bool onoff)
{
    if (d->m_marginSettings.m_useIndenter != onoff) {
        d->m_marginSettings.m_useIndenter = onoff;
        emit marginSettingsChanged(d->m_marginSettings);
    }
}

void EditorConfiguration::setWrapColumn(int column)
{
    if (d->m_marginSettings.m_marginColumn != column) {
        d->m_marginSettings.m_marginColumn = column;
        emit marginSettingsChanged(d->m_marginSettings);
    }
}

void EditorConfiguration::slotAboutToRemoveProject(Project *project)
{
    if (project->editorConfiguration() != this)
        return;

    for (BaseTextEditor *editor : std::as_const(d->m_editors))
        deconfigureEditor(editor);
}

TabSettings actualTabSettings(const Utils::FilePath &file,
                              const TextDocument *baseTextdocument)
{
    if (baseTextdocument)
        return baseTextdocument->tabSettings();
    if (Project *project = ProjectManager::projectForFile(file))
        return project->editorConfiguration()->codeStyle()->tabSettings();
    return TextEditorSettings::codeStyle()->tabSettings();
}

} // ProjectExplorer
