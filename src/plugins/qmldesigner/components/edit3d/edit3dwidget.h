// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <QLabel>
#include <QMenu>
#include <QPointer>
#include <QVector3D>
#include <QWidget>

#include <coreplugin/icontext.h>
#include "itemlibraryinfo.h"
#include <modelnode.h>

namespace QmlDesigner {

class Edit3DView;
class Edit3DCanvas;
class ToolBox;

struct ItemLibraryDetails {
    QString name;
    QIcon icon;
    QList<ItemLibraryEntry> entryList;

    ItemLibraryDetails(
            const QString &name = QString(),
            const QIcon &icon = QIcon())
        : name (name)
        , icon(icon)
    {}
};

class Edit3DWidget : public QWidget
{
    Q_OBJECT

public:
    Edit3DWidget(Edit3DView *view);

    Edit3DCanvas *canvas() const;
    Edit3DView *view() const;
    void contextHelp(const Core::IContext::HelpCallback &callback) const;

    void showCanvas(bool show);
    QMenu *visibilityTogglesMenu() const;
    void showVisibilityTogglesMenu(bool show, const QPoint &pos);

    QMenu *backgroundColorMenu() const;
    void showBackgroundColorMenu(bool show, const QPoint &pos);

    void showContextMenu(const QPoint &pos, const ModelNode &modelNode, const QVector3D &pos3d);
    void updateCreateSubMenu(const QList<ItemLibraryDetails> &entriesList);

private slots:
    void onCreateAction();

protected:
    void dragEnterEvent(QDragEnterEvent *dragEnterEvent) override;
    void dropEvent(QDropEvent *dropEvent) override;

private:
    void linkActivated(const QString &link);
    void createContextMenu();

    bool isPasteAvailable() const;
    bool isSceneLocked() const;

    void showOnboardingLabel();

    QPointer<Edit3DView> m_edit3DView;
    QPointer<Edit3DView> m_view;
    QPointer<Edit3DCanvas> m_canvas;
    QPointer<QLabel> m_onboardingLabel;
    QPointer<QLabel> m_mcuLabel;
    QPointer<ToolBox> m_toolBox;
    Core::IContext *m_context = nullptr;
    QPointer<QMenu> m_visibilityTogglesMenu;
    QPointer<QMenu> m_backgroundColorMenu;
    QPointer<QMenu> m_contextMenu;
    QPointer<QAction> m_bakeLightsAction;
    QPointer<QAction> m_editComponentAction;
    QPointer<QAction> m_editMaterialAction;
    QPointer<QAction> m_duplicateAction;
    QPointer<QAction> m_copyAction;
    QPointer<QAction> m_pasteAction;
    QPointer<QAction> m_deleteAction;
    QPointer<QAction> m_fitSelectedAction;
    QPointer<QAction> m_alignCameraAction;
    QPointer<QAction> m_alignViewAction;
    QPointer<QAction> m_selectParentAction;
    QPointer<QAction> m_toggleGroupAction;
    QPointer<QMenu> m_createSubMenu;
    ModelNode m_contextMenuTarget;
    QVector3D m_contextMenuPos3d;
    QHash<QString, ItemLibraryEntry> m_nameToEntry;
    ItemLibraryEntry m_draggedEntry;
};

} // namespace QmlDesigner
