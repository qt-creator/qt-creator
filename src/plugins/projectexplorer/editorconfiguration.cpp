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
** Nokia at qt-info@nokia.com.
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
#include <texteditor/storagesettings.h>
#include <texteditor/behaviorsettings.h>
#include <texteditor/extraencodingsettings.h>

#include <QtCore/QLatin1String>
#include <QtCore/QByteArray>
#include <QtCore/QTextCodec>

static const QLatin1String kPrefix("EditorConfiguration.");
static const QLatin1String kUseGlobal("EditorConfiguration.UseGlobal");
static const QLatin1String kCodec("EditorConfiguration.Codec");

using namespace TextEditor;

namespace ProjectExplorer {

struct EditorConfigurationPrivate
{
    EditorConfigurationPrivate()
        : m_useGlobal(true)
        , m_tabSettings(TextEditorSettings::instance()->tabSettings())
        , m_storageSettings(TextEditorSettings::instance()->storageSettings())
        , m_behaviorSettings(TextEditorSettings::instance()->behaviorSettings())
        , m_extraEncodingSettings(TextEditorSettings::instance()->extraEncodingSettings())
        , m_textCodec(Core::EditorManager::instance()->defaultTextCodec())
    {}

    bool m_useGlobal;
    TabSettings m_tabSettings;
    StorageSettings m_storageSettings;
    BehaviorSettings m_behaviorSettings;
    ExtraEncodingSettings m_extraEncodingSettings;
    QTextCodec *m_textCodec;
};

EditorConfiguration::EditorConfiguration() : m_d(new EditorConfigurationPrivate)
{
}

EditorConfiguration::~EditorConfiguration()
{
}

bool EditorConfiguration::useGlobalSettings() const
{
    return m_d->m_useGlobal;
}

void EditorConfiguration::cloneGlobalSettings()
{
    m_d->m_tabSettings = TextEditorSettings::instance()->tabSettings();
    m_d->m_storageSettings = TextEditorSettings::instance()->storageSettings();
    m_d->m_behaviorSettings = TextEditorSettings::instance()->behaviorSettings();
    m_d->m_extraEncodingSettings = TextEditorSettings::instance()->extraEncodingSettings();
    m_d->m_textCodec = Core::EditorManager::instance()->defaultTextCodec();

    emitTabSettingsChanged();
    emitStorageSettingsChanged();
    emitBehaviorSettingsChanged();
    emitExtraEncodingSettingsChanged();
}

QTextCodec *EditorConfiguration::textCodec() const
{
    return m_d->m_textCodec;
}

const TabSettings &EditorConfiguration::tabSettings() const
{
    return m_d->m_tabSettings;
}

const StorageSettings &EditorConfiguration::storageSettings() const
{
    return m_d->m_storageSettings;
}

const BehaviorSettings &EditorConfiguration::behaviorSettings() const
{
    return m_d->m_behaviorSettings;
}

const ExtraEncodingSettings &EditorConfiguration::extraEncodingSettings() const
{
    return m_d->m_extraEncodingSettings;
}

QVariantMap EditorConfiguration::toMap() const
{
    QVariantMap map;
    map.insert(kUseGlobal, m_d->m_useGlobal);
    map.insert(kCodec, m_d->m_textCodec->name());
    m_d->m_tabSettings.toMap(kPrefix, &map);
    m_d->m_storageSettings.toMap(kPrefix, &map);
    m_d->m_behaviorSettings.toMap(kPrefix, &map);
    m_d->m_extraEncodingSettings.toMap(kPrefix, &map);

    return map;
}

void EditorConfiguration::fromMap(const QVariantMap &map)
{
    m_d->m_useGlobal = map.value(kUseGlobal, m_d->m_useGlobal).toBool();

    const QByteArray &codecName = map.value(kCodec, m_d->m_textCodec->name()).toByteArray();
    m_d->m_textCodec = QTextCodec::codecForName(codecName);
    if (!m_d->m_textCodec)
        m_d->m_textCodec = Core::EditorManager::instance()->defaultTextCodec();

    m_d->m_tabSettings.fromMap(kPrefix, map);
    m_d->m_storageSettings.fromMap(kPrefix, map);
    m_d->m_behaviorSettings.fromMap(kPrefix, map);
    m_d->m_extraEncodingSettings.fromMap(kPrefix, map);
}

void EditorConfiguration::apply(ITextEditor *textEditor) const
{
    if (!m_d->m_useGlobal) {
        textEditor->setTextCodec(m_d->m_textCodec, ITextEditor::TextCodecFromProjectSetting);
        if (BaseTextEditorWidget *baseTextEditor = qobject_cast<BaseTextEditorWidget *>(textEditor->widget()))
            switchSettings(baseTextEditor);
    }
}

void EditorConfiguration::setUseGlobalSettings(bool use)
{
    m_d->m_useGlobal = use;
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
    if (m_d->m_useGlobal)
        switchSettings_helper(TextEditorSettings::instance(), this, baseTextEditor);
    else
        switchSettings_helper(this, TextEditorSettings::instance(), baseTextEditor);
}

template <class NewSenderT, class OldSenderT>
void EditorConfiguration::switchSettings_helper(const NewSenderT *newSender,
                                                const OldSenderT *oldSender,
                                                BaseTextEditorWidget *baseTextEditor) const
{
    baseTextEditor->setTabSettings(newSender->tabSettings());
    baseTextEditor->setStorageSettings(newSender->storageSettings());
    baseTextEditor->setBehaviorSettings(newSender->behaviorSettings());
    baseTextEditor->setExtraEncodingSettings(newSender->extraEncodingSettings());

    disconnect(oldSender, SIGNAL(tabSettingsChanged(TextEditor::TabSettings)),
               baseTextEditor, SLOT(setTabSettings(TextEditor::TabSettings)));
    disconnect(oldSender, SIGNAL(storageSettingsChanged(TextEditor::StorageSettings)),
               baseTextEditor, SLOT(setStorageSettings(TextEditor::StorageSettings)));
    disconnect(oldSender, SIGNAL(behaviorSettingsChanged(TextEditor::BehaviorSettings)),
               baseTextEditor, SLOT(setBehaviorSettings(TextEditor::BehaviorSettings)));
    disconnect(oldSender, SIGNAL(extraEncodingSettingsChanged(TextEditor::ExtraEncodingSettings)),
               baseTextEditor, SLOT(setExtraEncodingSettings(TextEditor::ExtraEncodingSettings)));

    connect(newSender, SIGNAL(tabSettingsChanged(TextEditor::TabSettings)),
            baseTextEditor, SLOT(setTabSettings(TextEditor::TabSettings)));
    connect(newSender, SIGNAL(storageSettingsChanged(TextEditor::StorageSettings)),
            baseTextEditor, SLOT(setStorageSettings(TextEditor::StorageSettings)));
    connect(newSender, SIGNAL(behaviorSettingsChanged(TextEditor::BehaviorSettings)),
            baseTextEditor, SLOT(setBehaviorSettings(TextEditor::BehaviorSettings)));
    connect(newSender, SIGNAL(extraEncodingSettingsChanged(TextEditor::ExtraEncodingSettings)),
            baseTextEditor, SLOT(setExtraEncodingSettings(TextEditor::ExtraEncodingSettings)));
}

void EditorConfiguration::setInsertSpaces(bool spaces)
{
    m_d->m_tabSettings.m_spacesForTabs = spaces;
    emitTabSettingsChanged();
}

void EditorConfiguration::setAutoInsertSpaces(bool autoSpaces)
{
    m_d->m_tabSettings.m_autoSpacesForTabs = autoSpaces;
    emitTabSettingsChanged();
}

void EditorConfiguration::setAutoIndent(bool autoIndent)
{
    m_d->m_tabSettings.m_autoIndent = autoIndent;
    emitTabSettingsChanged();
}

void EditorConfiguration::setSmartBackSpace(bool smartBackSpace)
{
    m_d->m_tabSettings.m_smartBackspace = smartBackSpace;
    emitTabSettingsChanged();
}

void EditorConfiguration::setTabSize(int size)
{
    m_d->m_tabSettings.m_tabSize = size;
    emitTabSettingsChanged();
}

void EditorConfiguration::setIndentSize(int size)
{
    m_d->m_tabSettings.m_indentSize = size;
    emitTabSettingsChanged();
}

void EditorConfiguration::setIndentBlocksBehavior(int index)
{
    m_d->m_tabSettings.m_indentBraces = index >= 1;
    m_d->m_tabSettings.m_doubleIndentBlocks = index >= 2;
    emitTabSettingsChanged();
}

void EditorConfiguration::setTabKeyBehavior(int index)
{
    m_d->m_tabSettings.m_tabKeyBehavior = (TabSettings::TabKeyBehavior)index;
    emitTabSettingsChanged();
}

void EditorConfiguration::setContinuationAlignBehavior(int index)
{
    m_d->m_tabSettings.m_continuationAlignBehavior = (TabSettings::ContinuationAlignBehavior)index;
    emitTabSettingsChanged();
}

void EditorConfiguration::setCleanWhiteSpace(bool cleanWhiteSpace)
{
    m_d->m_storageSettings.m_cleanWhitespace = cleanWhiteSpace;
    emitStorageSettingsChanged();
}

void EditorConfiguration::setInEntireDocument(bool entireDocument)
{
    m_d->m_storageSettings.m_inEntireDocument = entireDocument;
    emitStorageSettingsChanged();
}

void EditorConfiguration::setAddFinalNewLine(bool newLine)
{
    m_d->m_storageSettings.m_addFinalNewLine = newLine;
    emitStorageSettingsChanged();
}

void EditorConfiguration::setCleanIndentation(bool cleanIndentation)
{
    m_d->m_storageSettings.m_cleanIndentation = cleanIndentation;
    emitStorageSettingsChanged();
}

void EditorConfiguration::setMouseNavigation(bool mouseNavigation)
{
    m_d->m_behaviorSettings.m_mouseNavigation = mouseNavigation;
    emitBehaviorSettingsChanged();
}

void EditorConfiguration::setScrollWheelZooming(bool scrollZooming)
{
    m_d->m_behaviorSettings.m_scrollWheelZooming = scrollZooming;
    emitBehaviorSettingsChanged();
}

void EditorConfiguration::setUtf8BomSettings(int index)
{
    m_d->m_extraEncodingSettings.m_utf8BomSetting = (ExtraEncodingSettings::Utf8BomSetting)index;
    emitExtraEncodingSettingsChanged();
}

void EditorConfiguration::setTextCodec(QTextCodec *textCodec)
{
    m_d->m_textCodec = textCodec;
}

void EditorConfiguration::emitTabSettingsChanged()
{
    emit tabSettingsChanged(m_d->m_tabSettings);
}

void EditorConfiguration::emitStorageSettingsChanged()
{
    emit storageSettingsChanged(m_d->m_storageSettings);
}

void EditorConfiguration::emitBehaviorSettingsChanged()
{
    emit behaviorSettingsChanged(m_d->m_behaviorSettings);
}

void EditorConfiguration::emitExtraEncodingSettingsChanged()
{
    emit extraEncodingSettingsChanged(m_d->m_extraEncodingSettings);
}

const TabSettings &actualTabSettings(const QString &fileName, const BaseTextEditorWidget *baseTextEditor)
{
    if (baseTextEditor) {
        return baseTextEditor->tabSettings();
    } else {
        const SessionManager *session = ProjectExplorerPlugin::instance()->session();
        if (Project *project = session->projectForFile(fileName))
            return project->editorConfiguration()->tabSettings();
        else
            return TextEditorSettings::instance()->tabSettings();
    }
}

} // ProjectExplorer
