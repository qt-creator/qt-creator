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
        m_textEncoding(Core::EditorManager::defaultTextEncoding())
    { }

    ICodeStylePreferences *m_defaultCodeStyle = nullptr;
    bool m_useGlobal = true;
    TextEncoding m_textEncoding;

    QMap<Utils::Id, ICodeStylePreferences *> m_languageCodeStylePreferences;
    QList<Core::IEditor *> m_editors;
};

EditorConfiguration::EditorConfiguration()
    : marginSettings(kPrefix)
    , d(std::make_unique<EditorConfigurationPrivate>())
{
    behaviorSettings.setAutoApply(true);
    storageSettings.setAutoApply(true);
    extraEncodingSettings.setAutoApply(true);

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
    typingSettings.setData(globalTypingSettings().data());
    storageSettings.setData(globalStorageSettings().data());
    behaviorSettings.setData(globalBehaviorSettings().data());
    extraEncodingSettings.setData(globalExtraEncodingSettings().data());
    marginSettings.setData(TextEditor::marginSettings().data());
    d->m_textEncoding = Core::EditorManager::defaultTextEncoding();

    emit typingSettingsChanged(typingSettings.data());
    emit storageSettingsChanged(storageSettings.data());
    emit behaviorSettingsChanged(behaviorSettings.data());
    emit extraEncodingSettingsChanged(extraEncodingSettings.data());
}

TextEncoding EditorConfiguration::textEncoding() const
{
    return d->m_textEncoding;
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
        {kCodec, d->m_textEncoding.name()},
        {kCodeStyleCount, d->m_languageCodeStylePreferences.count()}
    };

    int i = 0;
    for (auto itCodeStyle = d->m_languageCodeStylePreferences.cbegin(),
               end = d->m_languageCodeStylePreferences.cend();
            itCodeStyle != end; ++itCodeStyle) {
        Store inner;
        itCodeStyle.value()->toMap(inner);
        const Store settingsIdMap = {
            {"language", QVariant::fromValue(itCodeStyle.key().toSetting())},
            {"value", QVariant::fromValue(inner)}
        };
        map.insert(numberedKey(kCodeStylePrefix, i), variantFromStore(settingsIdMap));
        i++;
    }

    Store inner;
    d->m_defaultCodeStyle->tabSettings().toMap(inner);
    typingSettings.toMap(inner);
    storageSettings.toMap(inner);
    behaviorSettings.toMap(inner);
    extraEncodingSettings.toMap(inner);
    toMapWithPrefix(&map, inner);
    marginSettings.toMap(map);

    return map;
}

void EditorConfiguration::fromMap(const Store &map)
{
    const QByteArray codecName = map.value(kCodec, d->m_textEncoding.name()).toByteArray();
    d->m_textEncoding = TextEncoding(codecName);
    if (!d->m_textEncoding.isValid())
        d->m_textEncoding = Core::EditorManager::defaultTextEncoding();

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
    typingSettings.fromMap(submap);
    storageSettings.fromMap(submap);
    behaviorSettings.fromMap(submap);
    extraEncodingSettings.fromMap(submap);
    marginSettings.fromMap(map);

    setUseGlobalSettings(map.value(kUseGlobal, d->m_useGlobal).toBool());
}

void EditorConfiguration::configureEditor(Core::IEditor *editor) const
{
    TextEditorWidget *widget = TextEditorWidget::fromEditor(editor);
    if (widget) {
        widget->textDocument()->setCodeStyle(codeStyle(widget->languageSettingsId()));
        if (!d->m_useGlobal) {
            widget->textDocument()->setEncoding(d->m_textEncoding);
            switchSettings(widget);
        }
    }
    d->m_editors.append(editor);
    connect(editor, &Core::IEditor::destroyed, this, [this, editor]() {
        d->m_editors.removeOne(editor);
    });
}

void EditorConfiguration::deconfigureEditor(Core::IEditor *editor) const
{
    TextEditorWidget *widget = TextEditorWidget::fromEditor(editor);
    if (widget)
        widget->textDocument()->setCodeStyle(TextEditorSettings::codeStyle(widget->languageSettingsId()));

    d->m_editors.removeOne(editor);

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
    QObject::disconnect(oldSender, &Old::typingSettingsChanged,
                        widget, &TextEditorWidget::setTypingSettings);
    QObject::disconnect(oldSender, &Old::storageSettingsChanged,
                        widget, &TextEditorWidget::setStorageSettings);
    QObject::disconnect(oldSender, &Old::behaviorSettingsChanged,
                        widget, &TextEditorWidget::setBehaviorSettings);
    QObject::disconnect(oldSender, &Old::extraEncodingSettingsChanged,
                        widget, &TextEditorWidget::setExtraEncodingSettings);

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
        widget->setMarginSettings(TextEditor::marginSettings().data());
        widget->setTypingSettings(globalTypingSettings().data());
        widget->setStorageSettings(globalStorageSettings().data());
        widget->setBehaviorSettings(globalBehaviorSettings().data());
        widget->setExtraEncodingSettings(globalExtraEncodingSettings().data());
        switchSettings_helper(TextEditorSettings::instance(), this, widget);
    } else {
        widget->setMarginSettings(marginSettings.data());
        widget->setTypingSettings(typingSettings.data());
        widget->setStorageSettings(storageSettings.data());
        widget->setBehaviorSettings(behaviorSettings.data());
        widget->setExtraEncodingSettings(extraEncodingSettings.data());
        switchSettings_helper(this, TextEditorSettings::instance(), widget);
    }
}

void EditorConfiguration::setTextEncoding(const TextEncoding &textEncoding)
{
    d->m_textEncoding = textEncoding;
}

void EditorConfiguration::slotAboutToRemoveProject(Project *project)
{
    if (project->editorConfiguration() != this)
        return;

    for (Core::IEditor *editor : std::as_const(d->m_editors))
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
