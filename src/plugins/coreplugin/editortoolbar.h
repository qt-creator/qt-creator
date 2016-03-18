/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "core_global.h"

#include <utils/styledbar.h>

#include <functional>

QT_BEGIN_NAMESPACE
class QMenu;
QT_END_NAMESPACE

namespace Core {
class IEditor;
class IDocument;

struct EditorToolBarPrivate;

/**
  * Fakes an IEditor-like toolbar for design mode widgets such as Qt Designer and Bauhaus.
  * Creates a combobox for open files and lock and close buttons on the right.
  */
class CORE_EXPORT EditorToolBar : public Utils::StyledBar
{
    Q_OBJECT

public:
    explicit EditorToolBar(QWidget *parent = 0);
    virtual ~EditorToolBar();

    typedef std::function<void(QMenu*)> MenuProvider;
    enum ToolbarCreationFlags { FlagsNone = 0, FlagsStandalone = 1 };

    /**
      * Adds an editor whose state is listened to, so that the toolbar can be kept up to date
      * with regards to locked status and tooltips.
      */
    void addEditor(IEditor *editor);

    /**
      * Sets the editor and adds its custom toolbar to the widget.
      */
    void setCurrentEditor(IEditor *editor);

    void setToolbarCreationFlags(ToolbarCreationFlags flags);
    void setMenuProvider(const MenuProvider &provider);

    /**
      * Adds a toolbar to the widget and sets invisible by default.
      */
    void addCenterToolBar(QWidget *toolBar);

    void setNavigationVisible(bool isVisible);
    void setCanGoBack(bool canGoBack);
    void setCanGoForward(bool canGoForward);
    void removeToolbarForEditor(IEditor *editor);
    void setCloseSplitEnabled(bool enable);
    void setCloseSplitIcon(const QIcon &icon);

signals:
    void closeClicked();
    void goBackClicked();
    void goForwardClicked();
    void horizontalSplitClicked();
    void verticalSplitClicked();
    void splitNewWindowClicked();
    void closeSplitClicked();
    void listSelectionActivated(int row);
    void currentDocumentMoved();

protected:
    bool eventFilter(QObject *obj, QEvent *event);

private:
    void changeActiveEditor(int row);
    void makeEditorWritable();

    void checkDocumentStatus();
    void closeEditor();
    void updateActionShortcuts();

    void updateDocumentStatus(IDocument *document);
    void updateEditorListSelection(IEditor *newSelection);
    void fillListContextMenu(QMenu *menu);
    void updateToolBar(QWidget *toolBar);

    EditorToolBarPrivate *d;
};

} // namespace Core
