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

#include "ui_stateview.h"

#include <QWidget>

namespace ScxmlEditor {

namespace PluginInterface {
class GraphicsScene;
class StateItem;
class ScxmlDocument;
class ScxmlUiFactory;
} // namespace PluginInterface

namespace Common {

class StateView : public QWidget
{
    Q_OBJECT

public:
    explicit StateView(PluginInterface::StateItem *state, QWidget *parent = nullptr);
    explicit StateView(QWidget *parent = nullptr);
    ~StateView() override;

    PluginInterface::StateItem *parentState() const;
    PluginInterface::GraphicsScene *scene() const;
    GraphicsView *view() const;
    void setUiFactory(PluginInterface::ScxmlUiFactory *uifactory);
    void setDocument(PluginInterface::ScxmlDocument *doc);
    void clear();

private:
    void closeView();
    void init();
    void initScene();

    PluginInterface::StateItem *m_parentState = nullptr;
    PluginInterface::GraphicsScene *m_scene = nullptr;
    bool m_isMainView;
    Ui::StateView m_ui;
};

} // namespace Common
} // namespace ScxmlEditor
