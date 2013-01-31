/**************************************************************************
**
** Copyright (c) 2013 Nicolas Arnaud-Cormos
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "macrooptionspage.h"

#include "macromanager.h"
#include "macrosconstants.h"
#include "macrooptionswidget.h"

#include <texteditor/texteditorconstants.h>

#include <QCoreApplication>
#include <QWidget>
#include <QIcon>

using namespace Macros;
using namespace Macros::Internal;


MacroOptionsPage::MacroOptionsPage(QObject *parent)
    : Core::IOptionsPage(parent)
{
    setId(Constants::M_OPTIONS_PAGE);
    setDisplayName(QCoreApplication::translate("Macros", Constants::M_OPTIONS_TR_PAGE));
    setCategory(TextEditor::Constants::TEXT_EDITOR_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("TextEditor",
        TextEditor::Constants::TEXT_EDITOR_SETTINGS_TR_CATEGORY));
}

QWidget *MacroOptionsPage::createPage(QWidget *parent)
{
    m_widget = new MacroOptionsWidget(parent);
    return m_widget;
}

void MacroOptionsPage::apply()
{
    if (m_widget)
        m_widget->apply();
}

void MacroOptionsPage::finish()
{
    // Nothing to do
}
