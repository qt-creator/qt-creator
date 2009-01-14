/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "formwindowhost.h"
#include "formeditorw.h"

#include <QtDesigner/QDesignerFormWindowInterface>
#include <QtDesigner/QDesignerFormEditorInterface>
#include <QtDesigner/QDesignerPropertyEditorInterface>

#include <QtCore/QDebug>
#include <QtCore/QVariant>

using namespace Designer::Internal;
using namespace SharedTools;

enum { debugFormWindowHost = 0 };

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
    if (debugFormWindowHost)
	qDebug() << "FormWindowHost::~FormWindowHost";	
}

void FormWindowHost::formSizeChanged(int w, int h)
{
    if (debugFormWindowHost)
	qDebug() << "FormWindowHost::formSizeChanged" << w << h;

    formWindow()->setDirty(true);
    static const QString geometry = QLatin1String("geometry");
    FormEditorW::instance()->designerEditor()->propertyEditor()->setPropertyValue(geometry, QRect(0,0,w,h) );
    emit changed();
}
