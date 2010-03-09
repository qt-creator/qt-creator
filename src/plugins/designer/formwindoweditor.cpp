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

#include "formwindoweditor.h"
#include "designerconstants.h"
#include "formeditorw.h"

#include <coreplugin/ifile.h>
#include <utils/qtcassert.h>

#include <QtDesigner/QDesignerFormWindowInterface>
#include <QtDesigner/QDesignerFormEditorInterface>
#include <QtDesigner/QDesignerFormWindowManagerInterface>
#include <QtDesigner/QDesignerPropertyEditorInterface>
#include "qt_private/formwindowbase_p.h"

#include <QtCore/QDebug>

using namespace Designer;
using namespace Designer::Internal;
using namespace Designer::Constants;

FormWindowEditor::FormWindowEditor(QDesignerFormWindowInterface *form,
                                   QWidget *parent) :
    SharedTools::WidgetHost(parent, form)
{
    connect(this, SIGNAL(formWindowSizeChanged(int,int)), this, SLOT(slotFormSizeChanged(int,int)));
}

void FormWindowEditor::slotFormSizeChanged(int w, int h)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << w << h;

    formWindow()->setDirty(true);
    static const QString geometry = QLatin1String("geometry");
    FormEditorW::instance()->designerEditor()->propertyEditor()->setPropertyValue(geometry, QRect(0,0,w,h) );
}
