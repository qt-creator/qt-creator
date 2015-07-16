/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "importmanagerview.h"
#include "importswidget.h"

#include <rewritingexception.h>
#include <qmldesignerplugin.h>

namespace QmlDesigner {

ImportManagerView::ImportManagerView(QObject *parent) :
    AbstractView(parent),
    m_importsWidget(0)

{
}

ImportManagerView::~ImportManagerView()
{

}

bool ImportManagerView::hasWidget() const
{
    return true;
}

WidgetInfo ImportManagerView::widgetInfo()
{
    if (m_importsWidget == 0) {
        m_importsWidget = new ImportsWidget;
        connect(m_importsWidget, SIGNAL(removeImport(Import)), this, SLOT(removeImport(Import)));
        connect(m_importsWidget, SIGNAL(addImport(Import)), this, SLOT(addImport(Import)));

        if (model())
            m_importsWidget->setImports(model()->imports());
    }

    return createWidgetInfo(m_importsWidget, 0, QLatin1String("ImportManager"), WidgetInfo::LeftPane, 1);
}

void ImportManagerView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);

    if (m_importsWidget) {
        m_importsWidget->setImports(model->imports());
        m_importsWidget->setPossibleImports(model->possibleImports());
        m_importsWidget->setUsedImports(model->usedImports());
    }
}

void ImportManagerView::modelAboutToBeDetached(Model *model)
{
    if (m_importsWidget) {
        m_importsWidget->removeImports();
        m_importsWidget->removePossibleImports();
        m_importsWidget->removeUsedImports();
    }

    AbstractView::modelAboutToBeDetached(model);
}

void ImportManagerView::nodeCreated(const ModelNode &/*createdNode*/)
{
    if (m_importsWidget)
        m_importsWidget->setUsedImports(model()->usedImports());
}

void ImportManagerView::nodeAboutToBeRemoved(const ModelNode &/*removedNode*/)
{
    if (m_importsWidget)
        m_importsWidget->setUsedImports(model()->usedImports());
}

void ImportManagerView::importsChanged(const QList<Import> &/*addedImports*/, const QList<Import> &/*removedImports*/)
{
    if (m_importsWidget) {
        m_importsWidget->setImports(model()->imports());
        m_importsWidget->setPossibleImports(model()->possibleImports());
        m_importsWidget->setUsedImports(model()->usedImports());
    }
}

void ImportManagerView::removeImport(const Import &import)
{
    try {
        if (model())
            model()->changeImports(QList<Import>(), QList<Import>() << import);
    }
    catch (const RewritingException &e) {
        e.showException();
    }
}

void ImportManagerView::addImport(const Import &import)
{
    try {
        if (model())
            model()->changeImports(QList<Import>() << import, QList<Import>());
    }
    catch (const RewritingException &e) {
        e.showException();
    }

    QmlDesignerPlugin::instance()->currentDesignDocument()->updateSubcomponentManager();
}

} // namespace QmlDesigner
