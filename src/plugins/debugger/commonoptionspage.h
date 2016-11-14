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

#include "debuggersourcepathmappingwidget.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <utils/savedaction.h>

#include <QPointer>
#include <QSharedPointer>

namespace Debugger {
namespace Internal {

class GlobalDebuggerOptions;
class DebuggerSourcePathMappingWidget;

///////////////////////////////////////////////////////////////////////
//
// CommonOptionsPage
//
///////////////////////////////////////////////////////////////////////

class CommonOptionsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    explicit CommonOptionsPage(const QSharedPointer<GlobalDebuggerOptions> &go);

    // IOptionsPage
    QWidget *widget() final;
    void apply() final;
    void finish() final;

    static QString msgSetBreakpointAtFunction(const char *function);
    static QString msgSetBreakpointAtFunctionToolTip(const char *function,
                                                     const QString &hint = QString());

private:
    QPointer<QWidget> m_widget;
    Utils::SavedActionSet m_group;
    const QSharedPointer<GlobalDebuggerOptions> m_options;
    DebuggerSourcePathMappingWidget *m_sourceMappingWidget = nullptr;
};


///////////////////////////////////////////////////////////////////////
//
// LocalsAndExpressionsOptionsPage
//
///////////////////////////////////////////////////////////////////////

class LocalsAndExpressionsOptionsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    LocalsAndExpressionsOptionsPage();

    // IOptionsPage
    QWidget *widget() final;
    void apply() final;
    void finish() final;

private:
    QPointer<QWidget> m_widget;
    Utils::SavedActionSet m_group;
};

} // namespace Internal
} // namespace Debugger
