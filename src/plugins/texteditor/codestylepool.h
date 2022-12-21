// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <QObject>

namespace Utils { class FilePath; }
namespace TextEditor {

class ICodeStylePreferences;
class ICodeStylePreferencesFactory;
class TabSettings;

namespace Internal { class CodeStylePoolPrivate; }

class TEXTEDITOR_EXPORT CodeStylePool : public QObject
{
    Q_OBJECT
public:
    explicit CodeStylePool(ICodeStylePreferencesFactory *factory, QObject *parent = nullptr);
    ~CodeStylePool() override;

    QList<ICodeStylePreferences *> codeStyles() const;
    QList<ICodeStylePreferences *> builtInCodeStyles() const;
    QList<ICodeStylePreferences *> customCodeStyles() const;

    ICodeStylePreferences *cloneCodeStyle(ICodeStylePreferences *originalCodeStyle);
    ICodeStylePreferences *createCodeStyle(const QByteArray &id, const TabSettings &tabSettings,
                      const QVariant &codeStyleData, const QString &displayName);
    // ownership is passed to the pool
    void addCodeStyle(ICodeStylePreferences *codeStyle);
    // is removed and deleted
    void removeCodeStyle(ICodeStylePreferences *codeStyle);

    ICodeStylePreferences *codeStyle(const QByteArray &id) const;

    void loadCustomCodeStyles();

    ICodeStylePreferences *importCodeStyle(const Utils::FilePath &fileName);
    void exportCodeStyle(const Utils::FilePath &fileName, ICodeStylePreferences *codeStyle) const;

signals:
    void codeStyleAdded(ICodeStylePreferences *);
    void codeStyleRemoved(ICodeStylePreferences *);

private:
    Utils::FilePath settingsDir() const;
    Utils::FilePath settingsPath(const QByteArray &id) const;
    ICodeStylePreferences *loadCodeStyle(const Utils::FilePath &fileName);
    void saveCodeStyle(ICodeStylePreferences *codeStyle) const;

    Internal::CodeStylePoolPrivate *d;
};

} // namespace TextEditor
