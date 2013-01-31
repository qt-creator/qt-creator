/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "designercontext.h"
#include "designerconstants.h"
#include "formeditorw.h"

#include <QDesignerFormEditorInterface>

#if QT_VERSION >= 0x050000
#    include <QDesignerIntegration>
#else
#    include "qt_private/qdesigner_integration_p.h"
#endif

#include <QWidget>
#include <QDebug>
#include <QSettings>

enum { debug = 0 };

namespace Designer {
namespace Internal {

DesignerContext::DesignerContext(const Core::Context &context,
                                 QWidget *widget, QObject *parent)
  : Core::IContext(parent)
{
    setContext(context);
    setWidget(widget);
}

QString DesignerContext::contextHelpId() const
{
    const QDesignerFormEditorInterface *core = FormEditorW::instance()->designerEditor();
#if QT_VERSION >= 0x050000
    return core->integration()->contextHelpId();
#else
    QString helpId;
     // Present from Qt 4.5.1 onwards. This will show the class documentation
    // scrolled to the current property.
    if (const qdesigner_internal::QDesignerIntegration *integration =
            qobject_cast<const qdesigner_internal::QDesignerIntegration*>(core->integration()))
        helpId = integration->contextHelpId();
    return helpId;
#endif
}

} // namespace Internal
} // namespace Designer
