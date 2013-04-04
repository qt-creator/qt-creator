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

#ifndef BEHAVIORSETTINGSWIDGET_H
#define BEHAVIORSETTINGSWIDGET_H

#include "texteditor_global.h"

#include <QWidget>

QT_BEGIN_NAMESPACE
class QTextCodec;
QT_END_NAMESPACE

namespace TextEditor {

class ICodeStylePreferences;
class TabSettingsWidget;
class TypingSettings;
class StorageSettings;
class BehaviorSettings;
class ExtraEncodingSettings;

struct BehaviorSettingsWidgetPrivate;

class TEXTEDITOR_EXPORT BehaviorSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BehaviorSettingsWidget(QWidget *parent = 0);
    virtual ~BehaviorSettingsWidget();

    void setActive(bool active);

    void setAssignedCodec(QTextCodec *codec);
    QTextCodec *assignedCodec() const;

    void setCodeStyle(ICodeStylePreferences *preferences);

    void setAssignedTypingSettings(const TypingSettings &typingSettings);
    void assignedTypingSettings(TypingSettings *typingSettings) const;

    void setAssignedStorageSettings(const StorageSettings &storageSettings);
    void assignedStorageSettings(StorageSettings *storageSettings) const;

    void setAssignedBehaviorSettings(const BehaviorSettings &behaviorSettings);
    void assignedBehaviorSettings(BehaviorSettings *behaviorSettings) const;

    void setAssignedExtraEncodingSettings(const ExtraEncodingSettings &encodingSettings);
    void assignedExtraEncodingSettings(ExtraEncodingSettings *encodingSettings) const;

    QString collectUiKeywords() const;

    TabSettingsWidget *tabSettingsWidget() const;

signals:
    void typingSettingsChanged(const TextEditor::TypingSettings &settings);
    void storageSettingsChanged(const TextEditor::StorageSettings &settings);
    void behaviorSettingsChanged(const TextEditor::BehaviorSettings &settings);
    void extraEncodingSettingsChanged(const TextEditor::ExtraEncodingSettings &settings);
    void textCodecChanged(QTextCodec *codec);

private slots:
    void slotTypingSettingsChanged();
    void slotStorageSettingsChanged();
    void slotBehaviorSettingsChanged();
    void slotExtraEncodingChanged();
    void slotEncodingBoxChanged(int index);

private:
    void updateConstrainTooltipsBoxTooltip() const;

    BehaviorSettingsWidgetPrivate *d;
};

} // TextEditor

#endif // BEHAVIORSETTINGSWIDGET_H
