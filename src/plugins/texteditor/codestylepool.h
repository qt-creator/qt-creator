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

#ifndef CODESTYLEPOOL_H
#define CODESTYLEPOOL_H

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

private slots:
    void slotSaveCodeStyle();

private:
    QString settingsDir() const;
    Utils::FileName settingsPath(const QByteArray &id) const;
    ICodeStylePreferences *loadCodeStyle(const Utils::FileName &fileName);
    void saveCodeStyle(ICodeStylePreferences *codeStyle) const;

    Internal::CodeStylePoolPrivate *d;
};

} // namespace TextEditor

#endif // CODESTYLEPOOL_H
