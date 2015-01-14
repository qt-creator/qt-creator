/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
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
