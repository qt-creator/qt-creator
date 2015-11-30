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
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#include "itemlibraryview.h"
#include "itemlibrarywidget.h"
#include <import.h>
#include <importmanagerview.h>

namespace QmlDesigner {

ItemLibraryView::ItemLibraryView(QObject* parent)
    : AbstractView(parent),
      m_importManagerView(new ImportManagerView(this))

{

}

ItemLibraryView::~ItemLibraryView()
{

}

bool ItemLibraryView::hasWidget() const
{
    return true;
}

WidgetInfo ItemLibraryView::widgetInfo()
{
    if (m_widget.isNull()) {
        m_widget = new ItemLibraryWidget;
        m_widget->setImportsWidget(m_importManagerView->widgetInfo().widget);
    }

    return createWidgetInfo(m_widget.data(),
                            new WidgetInfo::ToolBarWidgetDefaultFactory<ItemLibraryWidget>(m_widget.data()),
                            QStringLiteral("Library"),
                            WidgetInfo::LeftPane,
                            0);
}

void ItemLibraryView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);
    m_widget->setModel(model);
    updateImports();
    model->attachView(m_importManagerView);
}

void ItemLibraryView::modelAboutToBeDetached(Model *model)
{
    model->detachView(m_importManagerView);

    AbstractView::modelAboutToBeDetached(model);
    m_widget->setModel(0);
}

void ItemLibraryView::importsChanged(const QList<Import> &/*addedImports*/, const QList<Import> &/*removedImports*/)
{
    updateImports();
}

void ItemLibraryView::setResourcePath(const QString &resourcePath)
{
    if (m_widget.isNull())
        m_widget = new ItemLibraryWidget;

    m_widget->setResourcePath(resourcePath);
}

void ItemLibraryView::updateImports()
{
    m_widget->updateModel();
}

} //QmlDesigner
