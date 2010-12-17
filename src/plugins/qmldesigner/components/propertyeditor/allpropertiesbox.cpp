/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include <QtGui/QBoxLayout>

#include <model.h>

#include "application.h"
#include "allpropertiesbox.h"
#include "propertyeditor.h"

namespace QmlDesigner {

class AllPropertiesBoxPrivate
{
    friend class AllPropertiesBox;

private:
    PropertyEditor* propertiesEditor;
};

AllPropertiesBox::AllPropertiesBox(QWidget* parent):
        QStackedWidget(parent),
        m_d(new AllPropertiesBoxPrivate)
{
    m_d->propertiesEditor = new PropertyEditor(this);
    m_d->propertiesEditor->setQmlDir(Application::sharedDirPath() + QLatin1String("/propertyeditor"));

    m_newLookIndex = addWidget(m_d->propertiesEditor->createPropertiesPage());

    setCurrentIndex(m_newLookIndex);

    setWindowTitle(tr("Properties", "Title of properties view."));
}

AllPropertiesBox::~AllPropertiesBox()
{
    delete m_d;
}

void AllPropertiesBox::setModel(Model* model)
{
    if (model)
        model->attachView(m_d->propertiesEditor);
    else if (m_d->propertiesEditor->model())
        m_d->propertiesEditor->model()->detachView(m_d->propertiesEditor);
}

void AllPropertiesBox::showNewLook()
{
    setCurrentIndex(m_newLookIndex);
}

void AllPropertiesBox::showTraditional()
{
    setCurrentIndex(m_traditionalIndex);
}

}
