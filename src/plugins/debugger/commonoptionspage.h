/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef DEBUGGER_COMMONOPTIONSPAGE_H
#define DEBUGGER_COMMONOPTIONSPAGE_H

#include "ui_commonoptionspage.h"
#include "ui_localsandexpressionsoptionspage.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <utils/savedaction.h>

#include <QSharedPointer>
#include <QPointer>
#include <QWidget>

namespace Debugger {
namespace Internal {
class GlobalDebuggerOptions;

///////////////////////////////////////////////////////////////////////
//
// CommonOptionsPage
//
///////////////////////////////////////////////////////////////////////

class CommonOptionsPageWidget : public QWidget
{
public:
    explicit CommonOptionsPageWidget(const QSharedPointer<Utils::SavedActionSet> &group, QWidget *parent = 0);

    QString searchKeyWords() const;
    GlobalDebuggerOptions globalOptions() const;
    void setGlobalOptions(const GlobalDebuggerOptions &go);

private:
    Ui::CommonOptionsPage m_ui;
    const QSharedPointer<Utils::SavedActionSet> m_group;
};

class CommonOptionsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    explicit CommonOptionsPage(const QSharedPointer<GlobalDebuggerOptions> &go);
    virtual ~CommonOptionsPage();

    // IOptionsPage
    QString id() const;
    QString displayName() const;
    QString category() const;
    QString displayCategory() const;
    QIcon categoryIcon() const;
    QWidget *createPage(QWidget *parent);
    void apply();
    void finish();
    bool matches(const QString &s) const;

private:
    const QSharedPointer<GlobalDebuggerOptions> m_options;
    QSharedPointer<Utils::SavedActionSet> m_group;
    QString m_searchKeywords;
    QPointer<CommonOptionsPageWidget> m_widget;
};


///////////////////////////////////////////////////////////////////////
//
// LocalsAndExpressionsOptionsPage
//
///////////////////////////////////////////////////////////////////////

class LocalsAndExpressionsOptionsPage : public Core::IOptionsPage
{
public:
    LocalsAndExpressionsOptionsPage() {}

    // IOptionsPage
    QString id() const;
    QString displayName() const;
    QString category() const;
    QString displayCategory() const;
    QIcon categoryIcon() const;

    QWidget *createPage(QWidget *parent);
    void apply();
    void finish();
    bool matches(const QString &s) const;

private:
    Ui::DebuggingHelperOptionPage m_ui;
    Utils::SavedActionSet m_group;
    QString m_searchKeywords;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_COMMONOPTIONSPAGE_H
