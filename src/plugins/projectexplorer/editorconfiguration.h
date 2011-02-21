/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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

    bool useGlobalSettings() const;
    void cloneGlobalSettings();

    // The default codec is returned in the case the project doesn't override it.
    QTextCodec *textCodec() const;

    const TextEditor::TabSettings &tabSettings() const;
    const TextEditor::StorageSettings &storageSettings() const;
    const TextEditor::BehaviorSettings &behaviorSettings() const;
    const TextEditor::ExtraEncodingSettings &extraEncodingSettings() const;

    void apply(TextEditor::ITextEditor *textEditor) const;

    QVariantMap toMap() const;
    void fromMap(const QVariantMap &map);

signals:
    void tabSettingsChanged(const TextEditor::TabSettings &);
    void storageSettingsChanged(const TextEditor::StorageSettings &);
    void behaviorSettingsChanged(const TextEditor::BehaviorSettings &);
    void extraEncodingSettingsChanged(const TextEditor::ExtraEncodingSettings &);

private slots:
    void setUseGlobalSettings(bool use);

    void setInsertSpaces(bool spaces);
    void setAutoInsertSpaces(bool autoSpaces);
    void setAutoIndent(bool autoIndent);
    void setSmartBackSpace(bool smartBackSpace);
    void setTabSize(int size);
    void setIndentSize(int size);
    void setIndentBlocksBehavior(int index);
    void setTabKeyBehavior(int index);
    void setContinuationAlignBehavior(int index);

    void setCleanWhiteSpace(bool cleanWhiteSpace);
    void setInEntireDocument(bool entireDocument);
    void setAddFinalNewLine(bool newLine);
    void setCleanIndentation(bool cleanIndentation);

    void setMouseNavigation(bool mouseNavigation);
    void setScrollWheelZooming(bool scrollZooming);

    void setUtf8BomSettings(int index);

    void setTextCodec(QTextCodec *textCodec);

private:
    void switchSettings(TextEditor::BaseTextEditorWidget *baseTextEditor) const;
    template <class NewSenderT, class OldSenderT>
    void switchSettings_helper(const NewSenderT *newSender,
                               const OldSenderT *oldSender,
                               TextEditor::BaseTextEditorWidget *baseTextEditor) const;

    void emitTabSettingsChanged();
    void emitStorageSettingsChanged();
    void emitBehaviorSettingsChanged();
    void emitExtraEncodingSettingsChanged();

    QScopedPointer<EditorConfigurationPrivate> m_d;
};

// Return the editor settings in the case it's not null. Otherwise, try to find the project
// the file belongs to and return the project settings. If the file doesn't belong to any
// project return the global settings.
PROJECTEXPLORER_EXPORT const TextEditor::TabSettings &actualTabSettings(
    const QString &fileName, const TextEditor::BaseTextEditorWidget *baseTextEditor);

} // ProjectExplorer

#endif // EDITORCONFIGURATION_H
