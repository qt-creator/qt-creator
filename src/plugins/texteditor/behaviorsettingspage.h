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

#ifndef BEHAVIORSETTINGSPAGE_H
#define BEHAVIORSETTINGSPAGE_H

#include "texteditor_global.h"

#include "texteditoroptionspage.h"
#include "tabsettingswidget.h"

namespace TextEditor {

class TabSettings;
class TypingSettings;
class StorageSettings;
class BehaviorSettings;
class ExtraEncodingSettings;
class ICodeStylePreferences;
class CodeStylePool;

class BehaviorSettingsPageParameters
{
public:
    Core::Id id;
    QString displayName;
    QString settingsPrefix;
};

class BehaviorSettingsPage : public TextEditorOptionsPage
{
    Q_OBJECT

public:
    BehaviorSettingsPage(const BehaviorSettingsPageParameters &p, QObject *parent);
    ~BehaviorSettingsPage();

    // IOptionsPage
    QWidget *widget();
    void apply();
    void finish();

    ICodeStylePreferences *codeStyle() const;
    CodeStylePool *codeStylePool() const;
    const TypingSettings &typingSettings() const;
    const StorageSettings &storageSettings() const;
    const BehaviorSettings &behaviorSettings() const;
    const ExtraEncodingSettings &extraEncodingSettings() const;

signals:
    void typingSettingsChanged(const TextEditor::TypingSettings &);
    void storageSettingsChanged(const TextEditor::StorageSettings &);
    void behaviorSettingsChanged(const TextEditor::BehaviorSettings &);
    void extraEncodingSettingsChanged(const TextEditor::ExtraEncodingSettings &);

private slots:
    void openCodingStylePreferences(TextEditor::TabSettingsWidget::CodingStyleLink link);

private:
    void settingsFromUI(TypingSettings *typingSettings,
                        StorageSettings *storageSettings,
                        BehaviorSettings *behaviorSettings,
                        ExtraEncodingSettings *extraEncodingSettings) const;
    void settingsToUI();

    QList<QTextCodec *> m_codecs;
    struct BehaviorSettingsPagePrivate;
    BehaviorSettingsPagePrivate *d;
};

} // namespace TextEditor

#endif // BEHAVIORSETTINGSPAGE_H
