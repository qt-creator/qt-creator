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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef GDBOPTIONSPAGE_H
#define GDBOPTIONSPAGE_H

#include "ui_gdboptionspage.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <utils/savedaction.h>

namespace ProjectExplorer {
class ToolChain;
} // namespace ProjectExplorer

namespace Debugger {
namespace Internal {

class GdbOptionsPage : public Core::IOptionsPage
{
    Q_OBJECT
public:
    explicit GdbOptionsPage();

    virtual QString id() const { return settingsId(); }
    virtual QString displayName() const;
    virtual QString category() const;
    virtual QString displayCategory() const;
    virtual QIcon categoryIcon() const;

    virtual QWidget *createPage(QWidget *parent);
    virtual void apply();
    virtual void finish();
    virtual bool matches(const QString &) const;

    static QString settingsId();

private:
    Ui::GdbOptionsPage *m_ui;
    Utils::SavedActionSet m_group;
    QString m_searchKeywords;
};

} // namespace Internal
} // namespace Debugger

#endif // GDBOPTIONSPAGE_H
