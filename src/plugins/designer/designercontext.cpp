/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "designercontext.h"
#include "designerconstants.h"
#include "formeditorw.h"

#include <QtDesigner/QDesignerFormEditorInterface>
#include "qt_private/qdesigner_integration_p.h"

#include <QtGui/QWidget>
#include <QtCore/QDebug>

enum { debug = 0 };

namespace Designer {
namespace Internal {

DesignerContext::DesignerContext(const QList<int> contexts,
                                 QWidget *widget,
                                 QObject *parent) :
    Core::IContext(parent),
    m_context(contexts),
    m_widget(widget)
{
}

QList<int> DesignerContext::context() const
{
    return m_context;
}

QWidget *DesignerContext::widget()
{
    return m_widget;
}

QString DesignerContext::contextHelpId() const
{
    QString helpId;
    const QDesignerFormEditorInterface *core = FormEditorW::instance()->designerEditor();
    // Present from Qt 4.5.1 onwards. This will show the class documentation
    // scrolled to the current property.
    if (const qdesigner_internal::QDesignerIntegration *integration =
            qobject_cast<const qdesigner_internal::QDesignerIntegration*>(core->integration()))
        helpId = integration->contextHelpId();
    if (debug)
        qDebug() << "DesignerContext::contextHelpId" << m_widget << helpId;
    return helpId;
}

} // namespace Internal
} // namespace Designer
