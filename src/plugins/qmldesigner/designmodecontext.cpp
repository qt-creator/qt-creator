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

#include "designmodecontext.h"
#include "qmldesignerconstants.h"
#include "designmodewidget.h"
#include "formeditorwidget.h"
#include "edit3dwidget.h"
#include "navigatorwidget.h"
#include "texteditorwidget.h"

namespace QmlDesigner {
namespace Internal {

DesignModeContext::DesignModeContext(QWidget *widget)
  : IContext(widget)
{
    setWidget(widget);
    setContext(Core::Context(Constants::C_QMLDESIGNER, Constants::C_QT_QUICK_TOOLS_MENU));
}

void DesignModeContext::contextHelp(const HelpCallback &callback) const
{
    qobject_cast<DesignModeWidget *>(m_widget)->contextHelp(callback);
}

FormEditorContext::FormEditorContext(QWidget *widget)
  : IContext(widget)
{
    setWidget(widget);
    setContext(Core::Context(Constants::C_QMLFORMEDITOR, Constants::C_QT_QUICK_TOOLS_MENU));
}

void FormEditorContext::contextHelp(const HelpCallback &callback) const
{
    qobject_cast<FormEditorWidget *>(m_widget)->contextHelp(callback);
}

Editor3DContext::Editor3DContext(QWidget *widget)
  : IContext(widget)
{
    setWidget(widget);
    setContext(Core::Context(Constants::C_QMLEDITOR3D, Constants::C_QT_QUICK_TOOLS_MENU));
}

void Editor3DContext::contextHelp(const HelpCallback &callback) const
{
    qobject_cast<Edit3DWidget *>(m_widget)->contextHelp(callback);
}

NavigatorContext::NavigatorContext(QWidget *widget)
  : IContext(widget)
{
    setWidget(widget);
    setContext(Core::Context(Constants::C_QMLNAVIGATOR, Constants::C_QT_QUICK_TOOLS_MENU));
}

void NavigatorContext::contextHelp(const HelpCallback &callback) const
{
    qobject_cast<NavigatorWidget *>(m_widget)->contextHelp(callback);
}

TextEditorContext::TextEditorContext(QWidget *widget)
  : IContext(widget)
{
    setWidget(widget);
    setContext(Core::Context(Constants::C_QMLTEXTEDITOR, Constants::C_QT_QUICK_TOOLS_MENU));
}

void TextEditorContext::contextHelp(const HelpCallback &callback) const
{
    qobject_cast<TextEditorWidget *>(m_widget)->contextHelp(callback);
}

}
}

