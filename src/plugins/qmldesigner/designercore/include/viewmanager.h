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

#ifndef QMLDESIGNER_VIEWMANAGER_H
#define QMLDESIGNER_VIEWMANAGER_H

#include "abstractview.h"

#include <rewriterview.h>
#include <nodeinstanceview.h>
#include <itemlibraryview.h>
#include <navigatorview.h>
#include <stateseditorview.h>
#include <formeditorview.h>
#include <propertyeditor.h>
#include <componentview.h>
#include <model/viewlogger.h>

namespace QmlDesigner {

class DesignDocument;

namespace Internal {
    class DesignModeWidget;
}

class ViewManager
{
public:
    ViewManager();

    void attachRewriterView(TextModifier *textModifier);
    void detachRewriterView();

    void attachComponentView();
    void detachComponentView();

    void attachViewsExceptRewriterAndComponetView();
    void detachViewsExceptRewriterAndComponetView();

    void setItemLibraryViewResourcePath(const QString &resourcePath);
    void setComponentNode(const ModelNode &componentNode);
    void setNodeInstanceViewQtPath(const QString & qtPath);

    void resetPropertyEditorView();

    QWidget *formEditorWidget();
    QWidget *propertyEditorWidget();
    QWidget *itemLibraryWidget();
    QWidget *navigatorWidget();
    QWidget *statesEditorWidget();

    void pushFileOnCrambleBar(const QString &fileName);
    void pushInFileComponentOnCrambleBar(const QString &componentId);
    void nextFileIsCalledInternally();

private: // functions
    Q_DISABLE_COPY(ViewManager)

    void attachNodeInstanceView();
    void attachItemLibraryView();



    Model *currentModel() const;
    Model *documentModel() const;
    DesignDocument *currentDesignDocument() const;
    QString pathToQt() const;

    void switchStateEditorViewToBaseState();
    void switchStateEditorViewToSavedState();

private: // variables
    QmlModelState m_savedState;
    Internal::ViewLogger m_viewLogger;
    ComponentView m_componentView;
    FormEditorView m_formEditorView;
    ItemLibraryView m_itemLibraryView;
    NavigatorView m_navigatorView;
    PropertyEditor m_propertyEditorView;
    StatesEditorView m_statesEditorView;
    NodeInstanceView m_nodeInstanceView;

    QList<QWeakPointer<AbstractView> > m_additionalViews;
};

} // namespace QmlDesigner

#endif // QMLDESIGNER_VIEWMANAGER_H
