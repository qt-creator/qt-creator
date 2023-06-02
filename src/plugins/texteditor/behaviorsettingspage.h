// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

QT_BEGIN_NAMESPACE
class QTextCodec;
QT_END_NAMESPACE

namespace TextEditor {

class TabSettings;
class TypingSettings;
class StorageSettings;
class BehaviorSettings;
class ExtraEncodingSettings;
class ICodeStylePreferences;
class CodeStylePool;

class BehaviorSettingsPage : public Core::IOptionsPage
{
public:
    BehaviorSettingsPage();
    ~BehaviorSettingsPage() override;

    ICodeStylePreferences *codeStyle() const;
    CodeStylePool *codeStylePool() const;
    const TypingSettings &typingSettings() const;
    const StorageSettings &storageSettings() const;
    const BehaviorSettings &behaviorSettings() const;
    const ExtraEncodingSettings &extraEncodingSettings() const;

private:
    QList<QTextCodec *> m_codecs;
    class BehaviorSettingsPagePrivate *d;
};

} // namespace TextEditor
