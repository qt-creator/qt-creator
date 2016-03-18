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

#pragma once

#include "texteditor_global.h"

#include <QObject>

namespace Utils { class FileName; }
namespace TextEditor {

class ICodeStylePreferences;
class ICodeStylePreferencesFactory;
class TabSettings;

namespace Internal { class CodeStylePoolPrivate; }

class TEXTEDITOR_EXPORT CodeStylePool : public QObject
{
    Q_OBJECT
public:
    explicit CodeStylePool(ICodeStylePreferencesFactory *factory, QObject *parent = 0);
    virtual ~CodeStylePool();

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

    ICodeStylePreferences *importCodeStyle(const Utils::FileName &fileName);
    void exportCodeStyle(const Utils::FileName &fileName, ICodeStylePreferences *codeStyle) const;

signals:
    void codeStyleAdded(ICodeStylePreferences *);
    void codeStyleRemoved(ICodeStylePreferences *);

private:
    void slotSaveCodeStyle();

    QString settingsDir() const;
    Utils::FileName settingsPath(const QByteArray &id) const;
    ICodeStylePreferences *loadCodeStyle(const Utils::FileName &fileName);
    void saveCodeStyle(ICodeStylePreferences *codeStyle) const;

    Internal::CodeStylePoolPrivate *d;
};

} // namespace TextEditor
