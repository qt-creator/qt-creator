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

#include "designmodecontext.h"
#include "qmldesignerconstants.h"
#include "designmodewidget.h"
#include "formeditorwidget.h"

#include <QWidget>

namespace QmlDesigner {
namespace Internal {

DesignModeContext::DesignModeContext(DesignModeWidget *widget)
  : IContext(widget),
    m_widget(widget),
    m_context(Constants::C_QMLDESIGNER, Constants::C_QT_QUICK_TOOLS_MENU)
{
}

DesignModeContext::~DesignModeContext()
{
}

Core::Context DesignModeContext::context() const
{
    return m_context;
}

QWidget *DesignModeContext::widget()
{
    return m_widget;
}

QString DesignModeContext::contextHelpId() const
{
    return m_widget->contextHelpId();
}


FormEditorContext::FormEditorContext(FormEditorWidget *widget)
  : IContext(widget),
    m_widget(widget),
    m_context(Constants::C_QMLFORMEDITOR, Constants::C_QT_QUICK_TOOLS_MENU)
{
}

FormEditorContext::~FormEditorContext()
{
}

Core::Context FormEditorContext::context() const
{
    return m_context;
}

QWidget *FormEditorContext::widget()
{
    return m_widget;
}

QString FormEditorContext::contextHelpId() const
{
    return m_widget->contextHelpId();
}

}
}

