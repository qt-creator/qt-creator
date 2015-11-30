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

#ifndef QMLDESIGNER_VIEWMANAGER_H
#define QMLDESIGNER_VIEWMANAGER_H

#include "abstractview.h"

#include <QWidgetAction>

namespace ProjectExplorer {
class Kit;
}

namespace QmlDesigner {

class DesignDocument;
class AbstractCustomTool;
class DesignerActionManager;
class NodeInstanceView;

namespace Internal { class DesignModeWidget; }

class ViewManagerData;

class QMLDESIGNERCORE_EXPORT ViewManager
{
public:
    ViewManager();
    ~ViewManager();

    void attachRewriterView();
    void detachRewriterView();

    void attachComponentView();
    void detachComponentView();

    void attachViewsExceptRewriterAndComponetView();
    void detachViewsExceptRewriterAndComponetView();

    void setItemLibraryViewResourcePath(const QString &resourcePath);
    void setComponentNode(const ModelNode &componentNode);
    void setComponentViewToMaster();
    void setNodeInstanceViewKit(ProjectExplorer::Kit *kit);

    void resetPropertyEditorView();

    void registerFormEditorToolTakingOwnership(AbstractCustomTool *tool);
    void registerViewTakingOwnership(AbstractView *view);

    QList<WidgetInfo> widgetInfos();

    void disableWidgets();
    void enableWidgets();

    void pushFileOnCrumbleBar(const QString &fileName);
    void pushInFileComponentOnCrumbleBar(const ModelNode &modelNode);
    void nextFileIsCalledInternally();

    NodeInstanceView *nodeInstanceView() const;

    QWidgetAction *componentViewAction() const;

    DesignerActionManager &designerActionManager();
    const DesignerActionManager &designerActionManager() const;

private: // functions
    Q_DISABLE_COPY(ViewManager)

    void attachNodeInstanceView();
    void attachItemLibraryView();
    void attachAdditionalViews();
    void detachAdditionalViews();

    Model *currentModel() const;
    Model *documentModel() const;
    DesignDocument *currentDesignDocument() const;
    QString pathToQt() const;

    void switchStateEditorViewToBaseState();
    void switchStateEditorViewToSavedState();

private: // variables
    ViewManagerData *d;
};

} // namespace QmlDesigner

#endif // QMLDESIGNER_VIEWMANAGER_H
