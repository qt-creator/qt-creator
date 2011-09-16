/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "editorconfiguration.h"
#include "session.h"
#include "projectexplorer.h"
#include "project.h"

#include <coreplugin/editormanager/editormanager.h>

#include <texteditor/itexteditor.h>
#include <texteditor/basetexteditor.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/tabsettings.h>
#include <texteditor/tabpreferences.h>
#include <texteditor/storagesettings.h>
#include <texteditor/behaviorsettings.h>
#include <texteditor/extraencodingsettings.h>
#include <texteditor/codestylepreferencesmanager.h>
#include <texteditor/icodestylepreferencesfactory.h>

#include <QtCore/QLatin1String>
#include <QtCore/QByteArray>
#include <QtCore/QTextCodec>
#include <QtCore/QDebug>

static const QLatin1String kPrefix("EditorConfiguration.");
static const QLatin1String kUseGlobal("EditorConfiguration.UseGlobal");
static const QLatin1String kCodec("EditorConfiguration.Codec");
static const QLatin1String kTabPrefix("EditorConfiguration.Tab.");
static const QLatin1String kTabCount("EditorConfiguration.Tab.Count");
static const QLatin1String kCodeStylePrefix("EditorConfiguration.CodeStyle.");
static const QLatin1String kCodeStyleCount("EditorConfiguration.CodeStyle.Count");
static const QLatin1String kId("Project");

using namespace TextEditor;

namespace ProjectExplorer {

struct EditorConfigurationPrivate
{
    EditorConfigurationPrivate()
        : m_useGlobal(true)
        , m_tabPreferences(0)
        , m_storageSettings(TextEditorSettings::instance()->storageSettings())
        , m_behaviorSettings(TextEditorSettings::instance()->behaviorSettings())
        , m_extraEncodingSettings(TextEditorSettings::instance()->extraEncodingSettings())
        , m_textCodec(Core::EditorManager::instance()->defaultTextCodec())
    {
    }

    bool m_useGlobal;
    TabPreferences *m_tabPreferences;
    StorageSettings m_storageSettings;
    BehaviorSettings m_behaviorSettings;
    ExtraEncodingSettings m_extraEncodingSettings;
    QTextCodec *m_textCodec;

    QMap<QString, TabPreferences *> m_languageTabPreferences;
    QMap<QString, IFallbackPreferences *> m_languageCodeStylePreferences;
};

EditorConfiguration::EditorConfiguration() : d(new EditorConfigurationPrivate)
{
    TextEditor::TextEditorSettings *textEditorSettings = TextEditor::TextEditorSettings::instance();
    QList<TabPreferences *> tabFallbacks;
    tabFallbacks << textEditorSettings->tabPreferences();
    d->m_tabPreferences = new TabPreferences(tabFallbacks, this);
    d->m_tabPreferences->setFallbackEnabled(textEditorSettings->tabPreferences(), false);
    d->m_tabPreferences->setDisplayName(tr("Project", "Settings"));
    d->m_tabPreferences->setId(kId);

    CodeStylePreferencesManager *manager =
            CodeStylePreferencesManager::instance();
    TextEditorSettings *settings = TextEditorSettings::instance();

    const QMap<QString, TabPreferences *> languageTabPreferences = settings->languageTabPreferences();
    QMapIterator<QString, TabPreferences *> itTab(languageTabPreferences);
    while (itTab.hasNext()) {
        itTab.next();
        const QString languageId = itTab.key();
        TabPreferences *originalPreferences = itTab.value();
        QList<IFallbackPreferences *> fallbacks;
        fallbacks << originalPreferences->fallbacks();
        fallbacks << originalPreferences;
        fallbacks << tabPreferences();
        TabPreferences *preferences = new TabPreferences(
                    fallbacks, this);
        for (int i = 0; i < fallbacks.count(); i++)
            preferences->setFallbackEnabled(fallbacks.at(i), false);
        preferences->setId(languageId + QLatin1String("Project"));
        preferences->setCurrentFallback(originalPreferences);
        d->m_languageTabPreferences.insert(languageId, preferences);
    }

    const QMap<QString, IFallbackPreferences *> languageCodeStylePreferences = settings->languageCodeStylePreferences();
    QMapIterator<QString, IFallbackPreferences *> itCodeStyle(languageCodeStylePreferences);
    while (itCodeStyle.hasNext()) {
        itCodeStyle.next();
        const QString languageId = itCodeStyle.key();
        IFallbackPreferences *originalPreferences = itCodeStyle.value();
        ICodeStylePreferencesFactory *factory = manager->factory(languageId);
        IFallbackPreferences *preferences = factory->createPreferences(
                    QList<IFallbackPreferences *>() << originalPreferences);
        preferences->setFallbackEnabled(originalPreferences, false);
        preferences->setId(languageId + QLatin1String("Project"));
        preferences->setDisplayName(tr("Project %1", "Settings, %1 is a language (C++ or QML)").arg(factory->displayName()));
        preferences->setCurrentFallback(originalPreferences);
        d->m_languageCodeStylePreferences.insert(languageId, preferences);
    }
}

EditorConfiguration::~EditorConfiguration()
{
    qDeleteAll(d->m_languageTabPreferences.values());
    qDeleteAll(d->m_languageCodeStylePreferences.values());
    delete d;
}

bool EditorConfiguration::useGlobalSettings() const
{
    return d->m_useGlobal;
}

void EditorConfiguration::cloneGlobalSettings()
{
    d->m_tabPreferences->setSettings(TextEditorSettings::instance()->tabPreferences()->settings());
    setStorageSettings(TextEditorSettings::instance()->storageSettings());
    setBehaviorSettings(TextEditorSettings::instance()->behaviorSettings());
    setExtraEncodingSettings(TextEditorSettings::instance()->extraEncodingSettings());
    d->m_textCodec = Core::EditorManager::instance()->defaultTextCodec();
}

QTextCodec *EditorConfiguration::textCodec() const
{
    return d->m_textCodec;
}

TabPreferences *EditorConfiguration::tabPreferences() const
{
    return d->m_tabPreferences;
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

TabPreferences *EditorConfiguration::tabPreferences(const QString &languageId) const
{
    TabPreferences *prefs = d->m_languageTabPreferences.value(languageId);
    if (!prefs)
        prefs = d->m_tabPreferences;
    return prefs;
}

QMap<QString, TabPreferences *> EditorConfiguration::languageTabPreferences() const
{
    return d->m_languageTabPreferences;
}

IFallbackPreferences *EditorConfiguration::codeStylePreferences(const QString &languageId) const
{
    return d->m_languageCodeStylePreferences.value(languageId);
}

QMap<QString, IFallbackPreferences *> EditorConfiguration::languageCodeStylePreferences() const
{
    return d->m_languageCodeStylePreferences;
}

QVariantMap EditorConfiguration::toMap() const
{
    QVariantMap map;
    map.insert(kUseGlobal, d->m_useGlobal);
    map.insert(kCodec, d->m_textCodec->name());

    map.insert(kTabCount, d->m_languageTabPreferences.count());
    QMapIterator<QString, TabPreferences *> itTab(d->m_languageTabPreferences);
    int i = 0;
    while (itTab.hasNext()) {
        itTab.next();
        QVariantMap settingsIdMap;
        settingsIdMap.insert(QLatin1String("language"), itTab.key());
        QVariantMap value;
        itTab.value()->toMap(QString(), &value);
        settingsIdMap.insert(QLatin1String("value"), value);
        map.insert(kTabPrefix + QString::number(i), settingsIdMap);
        i++;
    }

    map.insert(kCodeStyleCount, d->m_languageCodeStylePreferences.count());
    QMapIterator<QString, IFallbackPreferences *> itCodeStyle(d->m_languageCodeStylePreferences);
    i = 0;
    while (itCodeStyle.hasNext()) {
        itCodeStyle.next();
        QVariantMap settingsIdMap;
        settingsIdMap.insert(QLatin1String("language"), itCodeStyle.key());
        QVariantMap value;
        itCodeStyle.value()->toMap(QString(), &value);
        settingsIdMap.insert(QLatin1String("value"), value);
        map.insert(kCodeStylePrefix + QString::number(i), settingsIdMap);
        i++;
    }

    d->m_tabPreferences->toMap(kPrefix, &map);
    d->m_storageSettings.toMap(kPrefix, &map);
    d->m_behaviorSettings.toMap(kPrefix, &map);
    d->m_extraEncodingSettings.toMap(kPrefix, &map);

    return map;
}

void EditorConfiguration::fromMap(const QVariantMap &map)
{
    d->m_useGlobal = map.value(kUseGlobal, d->m_useGlobal).toBool();

    const QByteArray &codecName = map.value(kCodec, d->m_textCodec->name()).toByteArray();
    d->m_textCodec = QTextCodec::codecForName(codecName);
    if (!d->m_textCodec)
        d->m_textCodec = Core::EditorManager::instance()->defaultTextCodec();

    const int tabCount = map.value(kTabCount, 0).toInt();
    for (int i = 0; i < tabCount; ++i) {
        QVariantMap settingsIdMap = map.value(kTabPrefix + QString::number(i)).toMap();
        if (settingsIdMap.isEmpty()) {
            qWarning() << "No data for tab settings list" << i << "found!";
            continue;
        }
        QString languageId = settingsIdMap.value(QLatin1String("language")).toString();
        QVariantMap value = settingsIdMap.value(QLatin1String("value")).toMap();
        TabPreferences *preferences = d->m_languageTabPreferences.value(languageId);
        if (preferences) {
             preferences->fromMap(QString(), value);
        }
    }

    const int codeStyleCount = map.value(kCodeStyleCount, 0).toInt();
    for (int i = 0; i < codeStyleCount; ++i) {
        QVariantMap settingsIdMap = map.value(kCodeStylePrefix + QString::number(i)).toMap();
        if (settingsIdMap.isEmpty()) {
            qWarning() << "No data for code style settings list" << i << "found!";
            continue;
        }
        QString languageId = settingsIdMap.value(QLatin1String("language")).toString();
        QVariantMap value = settingsIdMap.value(QLatin1String("value")).toMap();
        IFallbackPreferences *preferences = d->m_languageCodeStylePreferences.value(languageId);
        if (preferences) {
             preferences->fromMap(QString(), value);
        }
    }

    d->m_tabPreferences->fromMap(kPrefix, map);
    d->m_tabPreferences->setCurrentFallback(d->m_useGlobal
                    ? TextEditorSettings::instance()->tabPreferences() : 0);
    d->m_storageSettings.fromMap(kPrefix, map);
    d->m_behaviorSettings.fromMap(kPrefix, map);
    d->m_extraEncodingSettings.fromMap(kPrefix, map);
}

void EditorConfiguration::configureEditor(ITextEditor *textEditor) const
{
    BaseTextEditorWidget *baseTextEditor = qobject_cast<BaseTextEditorWidget *>(textEditor->widget());
    baseTextEditor->setTabPreferences(tabPreferences(baseTextEditor->languageSettingsId()));
    baseTextEditor->setCodeStylePreferences(codeStylePreferences(baseTextEditor->languageSettingsId()));
    if (!d->m_useGlobal) {
        textEditor->setTextCodec(d->m_textCodec, ITextEditor::TextCodecFromProjectSetting);
        if (baseTextEditor)
            switchSettings(baseTextEditor);
    }
}

void EditorConfiguration::setUseGlobalSettings(bool use)
{
    d->m_useGlobal = use;
    d->m_tabPreferences->setCurrentFallback(d->m_useGlobal
                    ? TextEditorSettings::instance()->tabPreferences() : 0);
    const SessionManager *session = ProjectExplorerPlugin::instance()->session();
    QList<Core::IEditor *> opened = Core::EditorManager::instance()->openedEditors();
    foreach (Core::IEditor *editor, opened) {
        if (BaseTextEditorWidget *baseTextEditor = qobject_cast<BaseTextEditorWidget *>(editor->widget())) {
            Project *project = session->projectForFile(editor->file()->fileName());
            if (project && project->editorConfiguration() == this)
                switchSettings(baseTextEditor);
        }
    }
}

void EditorConfiguration::switchSettings(BaseTextEditorWidget *baseTextEditor) const
{
    if (d->m_useGlobal)
        switchSettings_helper(TextEditorSettings::instance(), this, baseTextEditor);
    else
        switchSettings_helper(this, TextEditorSettings::instance(), baseTextEditor);
}

template <class NewSenderT, class OldSenderT>
void EditorConfiguration::switchSettings_helper(const NewSenderT *newSender,
                                                const OldSenderT *oldSender,
                                                BaseTextEditorWidget *baseTextEditor) const
{
    baseTextEditor->setStorageSettings(newSender->storageSettings());
    baseTextEditor->setBehaviorSettings(newSender->behaviorSettings());
    baseTextEditor->setExtraEncodingSettings(newSender->extraEncodingSettings());

    disconnect(oldSender, SIGNAL(storageSettingsChanged(TextEditor::StorageSettings)),
               baseTextEditor, SLOT(setStorageSettings(TextEditor::StorageSettings)));
    disconnect(oldSender, SIGNAL(behaviorSettingsChanged(TextEditor::BehaviorSettings)),
               baseTextEditor, SLOT(setBehaviorSettings(TextEditor::BehaviorSettings)));
    disconnect(oldSender, SIGNAL(extraEncodingSettingsChanged(TextEditor::ExtraEncodingSettings)),
               baseTextEditor, SLOT(setExtraEncodingSettings(TextEditor::ExtraEncodingSettings)));

    connect(newSender, SIGNAL(storageSettingsChanged(TextEditor::StorageSettings)),
            baseTextEditor, SLOT(setStorageSettings(TextEditor::StorageSettings)));
    connect(newSender, SIGNAL(behaviorSettingsChanged(TextEditor::BehaviorSettings)),
            baseTextEditor, SLOT(setBehaviorSettings(TextEditor::BehaviorSettings)));
    connect(newSender, SIGNAL(extraEncodingSettingsChanged(TextEditor::ExtraEncodingSettings)),
            baseTextEditor, SLOT(setExtraEncodingSettings(TextEditor::ExtraEncodingSettings)));
}

void EditorConfiguration::setStorageSettings(const TextEditor::StorageSettings &settings)
{
    d->m_storageSettings = settings;
    emit storageSettingsChanged(d->m_storageSettings);
}

void EditorConfiguration::setBehaviorSettings(const TextEditor::BehaviorSettings &settings)
{
    d->m_behaviorSettings = settings;
    emit behaviorSettingsChanged(d->m_behaviorSettings);
}

void EditorConfiguration::setExtraEncodingSettings(const TextEditor::ExtraEncodingSettings &settings)
{
    d->m_extraEncodingSettings = settings;
    emit extraEncodingSettingsChanged(d->m_extraEncodingSettings);
}

void EditorConfiguration::setTextCodec(QTextCodec *textCodec)
{
    d->m_textCodec = textCodec;
}

TabSettings actualTabSettings(const QString &fileName,
                                     const BaseTextEditorWidget *baseTextEditor)
{
    if (baseTextEditor) {
        return baseTextEditor->tabSettings();
    } else {
        const SessionManager *session = ProjectExplorerPlugin::instance()->session();
        if (Project *project = session->projectForFile(fileName))
            return project->editorConfiguration()->tabPreferences()->settings();
        else
            return TextEditorSettings::instance()->tabPreferences()->settings();
    }
}

} // ProjectExplorer
