/****************************************************************************
**
** Copyright (C) 2018 Sergey Morozov
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

#include <coreplugin/dialogs/ioptionspage.h>

#include <QPointer>

namespace Cppcheck {
namespace Internal {

class CppcheckTool;
class CppcheckTrigger;
class OptionsWidget;

class CppcheckOptions final
{
public:
    QString binary;

    bool warning = true;
    bool style = true;
    bool performance = true;
    bool portability = true;
    bool information = true;
    bool unusedFunction = false;
    bool missingInclude = false;
    bool inconclusive = false;
    bool forceDefines = false;

    QString customArguments;
    QString ignoredPatterns;
    bool showOutput = false;
    bool addIncludePaths = false;
    bool guessArguments = true;
};

class CppcheckOptionsPage final : public Core::IOptionsPage
{
    Q_OBJECT
public:
    explicit CppcheckOptionsPage(CppcheckTool &tool, CppcheckTrigger &trigger);

    QWidget *widget() final;
    void apply() final;
    void finish() final;

private:
    void save(const CppcheckOptions &options) const;
    void load(CppcheckOptions &options) const;

    CppcheckTool &m_tool;
    CppcheckTrigger &m_trigger;
    QPointer<OptionsWidget> m_widget;
};

} // namespace Internal
} // namespace Cppcheck
