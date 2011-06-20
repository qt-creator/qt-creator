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

#ifndef EDITORCONFIGURATION_H
#define EDITORCONFIGURATION_H

#include "projectexplorer_export.h"

#include <QtCore/QObject>
#include <QtCore/QVariantMap>
#include <QtCore/QScopedPointer>

namespace TextEditor {
class ITextEditor;
class BaseTextEditorWidget;
class TabSettings;
class TabPreferences;
class IFallbackPreferences;
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

    TextEditor::TabPreferences *tabPreferences() const;
    const TextEditor::StorageSettings &storageSettings() const;
    const TextEditor::BehaviorSettings &behaviorSettings() const;
    const TextEditor::ExtraEncodingSettings &extraEncodingSettings() const;

    TextEditor::TabPreferences *tabPreferences(const QString &languageId) const;
    QMap<QString, TextEditor::TabPreferences *> languageTabPreferences() const;
    TextEditor::IFallbackPreferences *codeStylePreferences(const QString &languageId) const;
    QMap<QString, TextEditor::IFallbackPreferences *> languageCodeStylePreferences() const;

    void configureEditor(TextEditor::ITextEditor *textEditor) const;

    QVariantMap toMap() const;
    void fromMap(const QVariantMap &map);

signals:
    void storageSettingsChanged(const TextEditor::StorageSettings &);
    void behaviorSettingsChanged(const TextEditor::BehaviorSettings &);
    void extraEncodingSettingsChanged(const TextEditor::ExtraEncodingSettings &);

private slots:

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

    QScopedPointer<EditorConfigurationPrivate> m_d;
};

// Return the editor settings in the case it's not null. Otherwise, try to find the project
// the file belongs to and return the project settings. If the file doesn't belong to any
// project return the global settings.
PROJECTEXPLORER_EXPORT TextEditor::TabSettings actualTabSettings(
    const QString &fileName, const TextEditor::BaseTextEditorWidget *baseTextEditor);

} // ProjectExplorer

#endif // EDITORCONFIGURATION_H
