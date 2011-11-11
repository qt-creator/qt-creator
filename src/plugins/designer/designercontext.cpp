/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "designercontext.h"
#include "designerconstants.h"
#include "formeditorw.h"

#include <QtDesigner/QDesignerFormEditorInterface>

#if QT_VERSION >= 0x050000
#    include <QtDesigner/QDesignerIntegration>
#else
#    include "qt_private/qdesigner_integration_p.h"
#endif

#include <QtGui/QWidget>
#include <QtCore/QDebug>
#include <QtCore/QSettings>

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
