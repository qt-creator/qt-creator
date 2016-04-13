/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "editorconfiguration.h"
#include "session.h"
#include "projectexplorer.h"
#include "project.h"

#include <utils/algorithm.h>

#include <coreplugin/id.h>
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

#include <QLatin1String>
#include <QByteArray>
#include <QTextCodec>
#include <QDebug>

static const QLatin1String kPrefix("EditorConfiguration.");
static const QLatin1String kUseGlobal("EditorConfiguration.UseGlobal");
static const QLatin1String kCodec("EditorConfiguration.Codec");
static const QLatin1String kCodeStylePrefix("EditorConfiguration.CodeStyle.");
static const QLatin1String kCodeStyleCount("EditorConfiguration.CodeStyle.Count");

using namespace TextEditor;

namespace ProjectExplorer {

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

    QMap<Core::Id, ICodeStylePreferences *> m_languageCodeStylePreferences;
    QList<BaseTextEditor *> m_editors;
};

EditorConfiguration::EditorConfiguration() : d(new EditorConfigurationPrivate)
{
    const QMap<Core::Id, ICodeStylePreferences *> languageCodeStylePreferences = TextEditorSettings::codeStyles();
    QMapIterator<Core::Id, ICodeStylePreferences *> itCodeStyle(languageCodeStylePreferences);
    while (itCodeStyle.hasNext()) {
        itCodeStyle.next();
        Core::Id languageId = itCodeStyle.key();
        // global prefs for language
        ICodeStylePreferences *originalPreferences = itCodeStyle.value();
        ICodeStylePreferencesFactory *factory = TextEditorSettings::codeStyleFactory(languageId);
        // clone of global prefs for language - it will became project prefs for language
        ICodeStylePreferences *preferences = factory->createCodeStyle();
        // project prefs can point to the global language pool, which contains also the global language prefs
        preferences->setDelegatingPool(TextEditorSettings::codeStylePool(languageId));
        preferences->setId(languageId.name() + "Project");
        preferences->setDisplayName(tr("Project %1", "Settings, %1 is a language (C++ or QML)").arg(factory->displayName()));
        // project prefs by default point to global prefs (which in turn can delegate to anything else or not)
        preferences->setCurrentDelegate(originalPreferences);
        d->m_languageCodeStylePreferences.insert(languageId, preferences);
    }

    // clone of global prefs (not language specific), for project scope
    d->m_defaultCodeStyle = new SimpleCodeStylePreferences(this);
    d->m_defaultCodeStyle->setDelegatingPool(TextEditorSettings::codeStylePool());
    d->m_defaultCodeStyle->setDisplayName(tr("Project", "Settings"));
    d->m_defaultCodeStyle->setId("Project");
    // if setCurrentDelegate is 0 values are read from *this prefs
    d->m_defaultCodeStyle->setCurrentDelegate(TextEditorSettings::codeStyle());

    connect(SessionManager::instance(), &SessionManager::aboutToRemoveProject,
            this, &EditorConfiguration::slotAboutToRemoveProject);
}

EditorConfiguration::~EditorConfiguration()
{
    qDeleteAll(d->m_languageCodeStylePreferences);
    delete d;
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
        settingsIdMap.insert(QLatin1String("language"), itCodeStyle.key().toSetting());
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
    d->m_marginSettings.toMap(kPrefix, &map);

    return map;
}

void EditorConfiguration::fromMap(const QVariantMap &map)
{
    const QByteArray &codecName = map.value(kCodec, d->m_textCodec->name()).toByteArray();
    d->m_textCodec = QTextCodec::codecForName(codecName);
    if (!d->m_textCodec)
        d->m_textCodec = Core::EditorManager::defaultTextCodec();

    const int codeStyleCount = map.value(kCodeStyleCount, 0).toInt();
    for (int i = 0; i < codeStyleCount; ++i) {
        QVariantMap settingsIdMap = map.value(kCodeStylePrefix + QString::number(i)).toMap();
        if (settingsIdMap.isEmpty()) {
            qWarning() << "No data for code style settings list" << i << "found!";
            continue;
        }
        Core::Id languageId = Core::Id::fromSetting(settingsIdMap.value(QLatin1String("language")));
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
    d->m_marginSettings.fromMap(kPrefix, map);
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
    d->m_defaultCodeStyle->setCurrentDelegate(use ? TextEditorSettings::codeStyle() : 0);
    foreach (Core::IEditor *editor, Core::DocumentModel::editorsForOpenedDocuments()) {
        if (auto widget = qobject_cast<TextEditorWidget *>(editor->widget())) {
            Project *project = SessionManager::projectForFile(editor->document()->filePath());
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

    foreach (BaseTextEditor *editor, d->m_editors)
        deconfigureEditor(editor);
}

TabSettings actualTabSettings(const QString &fileName,
                              const TextDocument *baseTextdocument)
{
    if (baseTextdocument)
        return baseTextdocument->tabSettings();
    if (Project *project = SessionManager::projectForFile(Utils::FileName::fromString(fileName)))
        return project->editorConfiguration()->codeStyle()->tabSettings();
    return TextEditorSettings::codeStyle()->tabSettings();
}

} // ProjectExplorer
