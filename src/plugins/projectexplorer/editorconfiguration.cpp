/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "editorconfiguration.h"
#include "session.h"
#include "projectexplorer.h"
#include "project.h"

#include <coreplugin/editormanager/editormanager.h>

#include <texteditor/itexteditor.h>
#include <texteditor/basetexteditor.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/tabsettings.h>
#include <texteditor/simplecodestylepreferences.h>
#include <texteditor/typingsettings.h>
#include <texteditor/storagesettings.h>
#include <texteditor/behaviorsettings.h>
#include <texteditor/extraencodingsettings.h>
#include <texteditor/icodestylepreferences.h>
#include <texteditor/icodestylepreferencesfactory.h>

#include <QLatin1String>
#include <QByteArray>
#include <QTextCodec>
#include <QDebug>

static const QLatin1String kPrefix("EditorConfiguration.");
static const QLatin1String kUseGlobal("EditorConfiguration.UseGlobal");
static const QLatin1String kCodec("EditorConfiguration.Codec");
static const QLatin1String kCodeStylePrefix("EditorConfiguration.CodeStyle.");
static const QLatin1String kCodeStyleCount("EditorConfiguration.CodeStyle.Count");
static const QLatin1String kId("Project");

using namespace TextEditor;

namespace ProjectExplorer {

struct EditorConfigurationPrivate
{
    EditorConfigurationPrivate()
        : m_useGlobal(true)
        , m_typingSettings(TextEditorSettings::instance()->typingSettings())
        , m_storageSettings(TextEditorSettings::instance()->storageSettings())
        , m_behaviorSettings(TextEditorSettings::instance()->behaviorSettings())
        , m_extraEncodingSettings(TextEditorSettings::instance()->extraEncodingSettings())
        , m_textCodec(Core::EditorManager::instance()->defaultTextCodec())
    {
    }

    bool m_useGlobal;
    ICodeStylePreferences *m_defaultCodeStyle;
    TypingSettings m_typingSettings;
    StorageSettings m_storageSettings;
    BehaviorSettings m_behaviorSettings;
    ExtraEncodingSettings m_extraEncodingSettings;
    QTextCodec *m_textCodec;

    QMap<Core::Id, ICodeStylePreferences *> m_languageCodeStylePreferences;
};

EditorConfiguration::EditorConfiguration() : d(new EditorConfigurationPrivate)
{
    TextEditorSettings *textEditorSettings = TextEditorSettings::instance();

    const QMap<Core::Id, ICodeStylePreferences *> languageCodeStylePreferences = textEditorSettings->codeStyles();
    QMapIterator<Core::Id, ICodeStylePreferences *> itCodeStyle(languageCodeStylePreferences);
    while (itCodeStyle.hasNext()) {
        itCodeStyle.next();
        Core::Id languageId = itCodeStyle.key();
        ICodeStylePreferences *originalPreferences = itCodeStyle.value();
        ICodeStylePreferencesFactory *factory = textEditorSettings->codeStyleFactory(languageId);
        ICodeStylePreferences *preferences = factory->createCodeStyle();
        preferences->setDelegatingPool(textEditorSettings->codeStylePool(languageId));
        preferences->setId(languageId.toString() + QLatin1String("Project"));
        preferences->setDisplayName(tr("Project %1", "Settings, %1 is a language (C++ or QML)").arg(factory->displayName()));
        preferences->setCurrentDelegate(originalPreferences);
        d->m_languageCodeStylePreferences.insert(languageId, preferences);
    }

    d->m_defaultCodeStyle = new SimpleCodeStylePreferences(this);
    d->m_defaultCodeStyle->setDelegatingPool(textEditorSettings->codeStylePool());
    d->m_defaultCodeStyle->setDisplayName(tr("Project", "Settings"));
    d->m_defaultCodeStyle->setId(kId);
    d->m_defaultCodeStyle->setCurrentDelegate(d->m_useGlobal
                    ? TextEditorSettings::instance()->codeStyle() : 0);
}

EditorConfiguration::~EditorConfiguration()
{
    qDeleteAll(d->m_languageCodeStylePreferences.values());
    delete d;
}

bool EditorConfiguration::useGlobalSettings() const
{
    return d->m_useGlobal;
}

void EditorConfiguration::cloneGlobalSettings()
{
    TextEditorSettings *textEditorSettings = TextEditorSettings::instance();

    d->m_defaultCodeStyle->setTabSettings(textEditorSettings->codeStyle()->tabSettings());
    setTypingSettings(textEditorSettings->typingSettings());
    setStorageSettings(textEditorSettings->storageSettings());
    setBehaviorSettings(textEditorSettings->behaviorSettings());
    setExtraEncodingSettings(textEditorSettings->extraEncodingSettings());
    d->m_textCodec = Core::EditorManager::instance()->defaultTextCodec();
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

ICodeStylePreferences *EditorConfiguration::codeStyle() const
{
    return d->m_defaultCodeStyle;
}

ICodeStylePreferences *EditorConfiguration::codeStyle(Core::Id languageId) const
{
    return d->m_languageCodeStylePreferences.value(languageId, codeStyle());
}

QMap<Core::Id, ICodeStylePreferences *> EditorConfiguration::codeStyles() const
{
    return d->m_languageCodeStylePreferences;
}

QVariantMap EditorConfiguration::toMap() const
{
    QVariantMap map;
    map.insert(kUseGlobal, d->m_useGlobal);
    map.insert(kCodec, d->m_textCodec->name());

    map.insert(kCodeStyleCount, d->m_languageCodeStylePreferences.count());
    QMapIterator<Core::Id, ICodeStylePreferences *> itCodeStyle(d->m_languageCodeStylePreferences);
    int i = 0;
    while (itCodeStyle.hasNext()) {
        itCodeStyle.next();
        QVariantMap settingsIdMap;
        settingsIdMap.insert(QLatin1String("language"), itCodeStyle.key().name());
        QVariantMap value;
        itCodeStyle.value()->toMap(QString(), &value);
        settingsIdMap.insert(QLatin1String("value"), value);
        map.insert(kCodeStylePrefix + QString::number(i), settingsIdMap);
        i++;
    }

    d->m_defaultCodeStyle->tabSettings().toMap(kPrefix, &map);
    d->m_typingSettings.toMap(kPrefix, &map);
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

    const int codeStyleCount = map.value(kCodeStyleCount, 0).toInt();
    for (int i = 0; i < codeStyleCount; ++i) {
        QVariantMap settingsIdMap = map.value(kCodeStylePrefix + QString::number(i)).toMap();
        if (settingsIdMap.isEmpty()) {
            qWarning() << "No data for code style settings list" << i << "found!";
            continue;
        }
        Core::Id languageId(settingsIdMap.value(QLatin1String("language")).toByteArray());
        QVariantMap value = settingsIdMap.value(QLatin1String("value")).toMap();
        ICodeStylePreferences *preferences = d->m_languageCodeStylePreferences.value(languageId);
        if (preferences)
             preferences->fromMap(QString(), value);
    }

    d->m_defaultCodeStyle->fromMap(kPrefix, map);
    d->m_typingSettings.fromMap(kPrefix, map);
    d->m_storageSettings.fromMap(kPrefix, map);
    d->m_behaviorSettings.fromMap(kPrefix, map);
    d->m_extraEncodingSettings.fromMap(kPrefix, map);
}

void EditorConfiguration::configureEditor(ITextEditor *textEditor) const
{
    BaseTextEditorWidget *baseTextEditor = qobject_cast<BaseTextEditorWidget *>(textEditor->widget());
    if (baseTextEditor)
        baseTextEditor->setCodeStyle(codeStyle(baseTextEditor->languageSettingsId()));
    if (!d->m_useGlobal) {
        textEditor->setTextCodec(d->m_textCodec, ITextEditor::TextCodecFromProjectSetting);
        if (baseTextEditor)
            switchSettings(baseTextEditor);
    }
}

void EditorConfiguration::setUseGlobalSettings(bool use)
{
    d->m_useGlobal = use;
    d->m_defaultCodeStyle->setCurrentDelegate(d->m_useGlobal
                    ? TextEditorSettings::instance()->codeStyle() : 0);
    const SessionManager *session = ProjectExplorerPlugin::instance()->session();
    QList<Core::IEditor *> opened = Core::EditorManager::instance()->openedEditors();
    foreach (Core::IEditor *editor, opened) {
        if (BaseTextEditorWidget *baseTextEditor = qobject_cast<BaseTextEditorWidget *>(editor->widget())) {
            Project *project = session->projectForFile(editor->document()->fileName());
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
    baseTextEditor->setTypingSettings(newSender->typingSettings());
    baseTextEditor->setStorageSettings(newSender->storageSettings());
    baseTextEditor->setBehaviorSettings(newSender->behaviorSettings());
    baseTextEditor->setExtraEncodingSettings(newSender->extraEncodingSettings());

    disconnect(oldSender, SIGNAL(typingSettingsChanged(TextEditor::TypingSettings)),
               baseTextEditor, SLOT(setTypingSettings(TextEditor::TypingSettings)));
    disconnect(oldSender, SIGNAL(storageSettingsChanged(TextEditor::StorageSettings)),
               baseTextEditor, SLOT(setStorageSettings(TextEditor::StorageSettings)));
    disconnect(oldSender, SIGNAL(behaviorSettingsChanged(TextEditor::BehaviorSettings)),
               baseTextEditor, SLOT(setBehaviorSettings(TextEditor::BehaviorSettings)));
    disconnect(oldSender, SIGNAL(extraEncodingSettingsChanged(TextEditor::ExtraEncodingSettings)),
               baseTextEditor, SLOT(setExtraEncodingSettings(TextEditor::ExtraEncodingSettings)));

    connect(newSender, SIGNAL(typingSettingsChanged(TextEditor::TypingSettings)),
            baseTextEditor, SLOT(setTypingSettings(TextEditor::TypingSettings)));
    connect(newSender, SIGNAL(storageSettingsChanged(TextEditor::StorageSettings)),
            baseTextEditor, SLOT(setStorageSettings(TextEditor::StorageSettings)));
    connect(newSender, SIGNAL(behaviorSettingsChanged(TextEditor::BehaviorSettings)),
            baseTextEditor, SLOT(setBehaviorSettings(TextEditor::BehaviorSettings)));
    connect(newSender, SIGNAL(extraEncodingSettingsChanged(TextEditor::ExtraEncodingSettings)),
            baseTextEditor, SLOT(setExtraEncodingSettings(TextEditor::ExtraEncodingSettings)));
}

void EditorConfiguration::setTypingSettings(const TextEditor::TypingSettings &settings)
{
    d->m_typingSettings = settings;
    emit typingSettingsChanged(d->m_typingSettings);
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
            return project->editorConfiguration()->codeStyle()->tabSettings();
        else
            return TextEditorSettings::instance()->codeStyle()->tabSettings();
    }
}

} // ProjectExplorer
