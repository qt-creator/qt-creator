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

#ifndef EDITORCONFIGURATION_H
#define EDITORCONFIGURATION_H

#include "projectexplorer_export.h"

#include <coreplugin/id.h>

#include <QObject>
#include <QVariantMap>

namespace TextEditor {
class ITextEditor;
class BaseTextEditorWidget;
class TabSettings;
class ICodeStylePreferences;
class TypingSettings;
class StorageSettings;
class BehaviorSettings;
class ExtraEncodingSettings;
}

namespace ProjectExplorer {

struct EditorConfigurationPrivate;

class PROJECTEXPLORER_EXPORT EditorConfiguration : public QObject
{
    Q_OBJECT

public:
    EditorConfiguration();
    ~EditorConfiguration();

    void setUseGlobalSettings(bool use);
    bool useGlobalSettings() const;
    void cloneGlobalSettings();

    // The default codec is returned in the case the project doesn't override it.
    QTextCodec *textCodec() const;

    const TextEditor::TypingSettings &typingSettings() const;
    const TextEditor::StorageSettings &storageSettings() const;
    const TextEditor::BehaviorSettings &behaviorSettings() const;
    const TextEditor::ExtraEncodingSettings &extraEncodingSettings() const;

    TextEditor::ICodeStylePreferences *codeStyle() const;
    TextEditor::ICodeStylePreferences *codeStyle(Core::Id languageId) const;
    QMap<Core::Id, TextEditor::ICodeStylePreferences *> codeStyles() const;

    void configureEditor(TextEditor::ITextEditor *textEditor) const;

    QVariantMap toMap() const;
    void fromMap(const QVariantMap &map);

signals:
    void typingSettingsChanged(const TextEditor::TypingSettings &);
    void storageSettingsChanged(const TextEditor::StorageSettings &);
    void behaviorSettingsChanged(const TextEditor::BehaviorSettings &);
    void extraEncodingSettingsChanged(const TextEditor::ExtraEncodingSettings &);

private slots:

    void setTypingSettings(const TextEditor::TypingSettings &settings);
    void setStorageSettings(const TextEditor::StorageSettings &settings);
    void setBehaviorSettings(const TextEditor::BehaviorSettings &settings);
    void setExtraEncodingSettings(const TextEditor::ExtraEncodingSettings &settings);

    void setTextCodec(QTextCodec *textCodec);

private:
    void switchSettings(TextEditor::BaseTextEditorWidget *baseTextEditor) const;
    template <class NewSenderT, class OldSenderT>
    void switchSettings_helper(const NewSenderT *newSender,
                               const OldSenderT *oldSender,
                               TextEditor::BaseTextEditorWidget *baseTextEditor) const;

    EditorConfigurationPrivate *d;
};

// Return the editor settings in the case it's not null. Otherwise, try to find the project
// the file belongs to and return the project settings. If the file doesn't belong to any
// project return the global settings.
PROJECTEXPLORER_EXPORT TextEditor::TabSettings actualTabSettings(
    const QString &fileName, const TextEditor::BaseTextEditorWidget *baseTextEditor);

} // ProjectExplorer

#endif // EDITORCONFIGURATION_H
