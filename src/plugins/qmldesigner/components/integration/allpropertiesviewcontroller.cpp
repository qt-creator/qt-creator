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

#include <QtGui/QStackedWidget>

#include <allpropertiesbox.h>

#include "designdocumentcontroller.h"
#include "allpropertiesviewcontroller.h"

namespace QmlDesigner {

AllPropertiesViewController::AllPropertiesViewController(QObject* parent):
        StackedUtilityPanelController(parent)
{
    setObjectName("PropertiesViewController");
}

QWidget* AllPropertiesViewController::stackedPageWidget(DesignDocumentController* designDocumentController) const
{
    return designDocumentController->allPropertiesBox();
}

void AllPropertiesViewController::showNewLookProperties()
{
    AllPropertiesBox* propsBox = dynamic_cast<AllPropertiesBox*>(dynamic_cast<QStackedWidget*>(contentWidget())->currentWidget());
    if (propsBox)
        propsBox->showNewLook();
}

void AllPropertiesViewController::showTraditionalProperties()
{
    AllPropertiesBox* propsBox = dynamic_cast<AllPropertiesBox*>(dynamic_cast<QStackedWidget*>(contentWidget())->currentWidget());
    if (propsBox)
        propsBox->showTraditional();
}

} // namespace QmlDesigner
