/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "formwindowhost.h"
#include "formeditorw.h"

#include <QtDesigner/QDesignerFormWindowInterface>
#include <QtDesigner/QDesignerFormEditorInterface>
#include <QtDesigner/QDesignerPropertyEditorInterface>

#include <QtCore/QDebug>
#include <QtCore/QVariant>

using namespace Designer::Internal;
using namespace SharedTools;

FormWindowHost::FormWindowHost(QDesignerFormWindowInterface *form,
                               QWidget *parent) :
    WidgetHost(parent, form)
{
    connect(formWindow(), SIGNAL(selectionChanged()), this, SIGNAL(changed()));
    connect(this, SIGNAL(formWindowSizeChanged(int,int)), this, SLOT(formSizeChanged(int,int)));
    connect(formWindow(), SIGNAL(changed()), this, SIGNAL(changed()));
}

FormWindowHost::~FormWindowHost()
{
    if (Designer::Constants::Internal::debug)
	qDebug() << Q_FUNC_INFO;
}

void FormWindowHost::formSizeChanged(int w, int h)
{
    if (Designer::Constants::Internal::debug)
	qDebug() << Q_FUNC_INFO << w << h;

    formWindow()->setDirty(true);
    static const QString geometry = QLatin1String("geometry");
    FormEditorW::instance()->designerEditor()->propertyEditor()->setPropertyValue(geometry, QRect(0,0,w,h) );
    emit changed();
}
