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

#include "mytypes.h"
#include "scxmldocument.h"

#include <QToolButton>
#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QFileInfo)
QT_FORWARD_DECLARE_CLASS(QResizeEvent)
QT_FORWARD_DECLARE_CLASS(QSplitter)
QT_FORWARD_DECLARE_CLASS(QStackedWidget)
QT_FORWARD_DECLARE_CLASS(QUndoStack)

namespace ScxmlEditor {

namespace PluginInterface {
class ActionHandler;
class BaseItem;
class ScxmlUiFactory;
} // namespace PluginInterface

namespace OutputPane {
class ErrorWidget;
class OutputTabWidget;
class WarningModel;
} // namespace OutputPane

namespace Common {

class ColorThemes;
class Magnifier;
class Navigator;
class Search;
class ShapesToolbox;
class StateProperties;
class StateView;
class Structure;

/**
 * @brief The MainWidget class is the main widget of the application.
 *
 * The responsibility of this class is to init all necessary widgets,
 * connect them each other and handle save-logic of the current document.
 *
 * When current document changes, the signal scxmlDocumentChanged(ScxmlDocument*) will be emitted.
 */
class MainWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MainWidget(QWidget *parent = nullptr);
    ~MainWidget() override;

    QAction *action(PluginInterface::ActionType act);
    QToolButton *toolButton(PluginInterface::ToolButtonType type);

    QString fileName() const;
    void setFileName(const QString &filename);
    QString errorMessage() const;
    QString contents() const;
    QUndoStack *undoStack() const;
    bool isDirty() const;
    void newDocument();
    void refresh();
    OutputPane::WarningModel *warningModel() const;
    PluginInterface::ScxmlUiFactory *uiFactory() const;
    bool load(const QString &fileName);
    bool save();
    void addStateView(PluginInterface::BaseItem *item = nullptr);
    void initView(int id);
    void fitToView();

protected:
    void showEvent(QShowEvent*) override;
    void resizeEvent(QResizeEvent *e) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *e) override;
    bool event(QEvent *e) override;

signals:
    void dirtyChanged(bool dirty);

private:
    QToolButton *createToolButton(const QIcon &icon, const QString &tooltip, QToolButton::ToolButtonPopupMode mode);
    void documentChanged();
    void createUi();
    void init();
    void clear();
    void handleTabVisibilityChanged(bool visible);
    void setMagnifier(bool m);
    void alignButtonClicked(PluginInterface::ActionType align);
    void adjustButtonClicked(PluginInterface::ActionType alignType);
    void endTagChange(PluginInterface::ScxmlDocument::TagChange change, const PluginInterface::ScxmlTag *tag, const QVariant &value);
    void exportToImage();
    void saveScreenShot();
    void saveSettings();

    Navigator *m_navigator = nullptr;
    Magnifier *m_magnifier = nullptr;
    OutputPane::ErrorWidget *m_errorPane = nullptr;
    Search *m_searchPane = nullptr;
    ColorThemes *m_colorThemes = nullptr;

    PluginInterface::ScxmlDocument *m_document = nullptr;
    PluginInterface::ScxmlUiFactory *m_uiFactory = nullptr;
    QVector<QToolButton*> m_toolButtons;
    QVector<StateView*> m_views;
    bool m_autoFit = true;
    bool m_windowBlocked = false;
    PluginInterface::ActionHandler *m_actionHandler = nullptr;
    QStackedWidget *m_stackedWidget = nullptr;
    OutputPane::OutputTabWidget *m_outputPaneWindow = nullptr;
    StateProperties *m_stateProperties = nullptr;
    Structure *m_structure = nullptr;
    QSplitter *m_horizontalSplitter = nullptr;
    QWidget *m_mainContentWidget = nullptr;
    ShapesToolbox *m_shapesFrame = nullptr;
};

} // namespace Common
} // namespace ScxmlEditor
