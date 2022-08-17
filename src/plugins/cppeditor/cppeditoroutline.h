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

#include "cppoverviewmodel.h"

#include <QModelIndex>
#include <QObject>

#include <memory>

QT_BEGIN_NAMESPACE
class QAction;
class QSortFilterProxyModel;
class QTimer;
QT_END_NAMESPACE

namespace TextEditor { class TextEditorWidget; }
namespace Utils { class TreeViewComboBox; }

namespace CppEditor {
class CppEditorWidget;

namespace Internal {

class CppEditorOutline : public QObject
{
    Q_OBJECT

public:
    explicit CppEditorOutline(CppEditorWidget *editorWidget);

    QWidget *widget() const; // Must be deleted by client.

public slots:
    void updateIndex();

private:
    void updateNow();
    void updateIndexNow();
    void updateToolTip();
    void gotoSymbolInEditor();

    CppEditorOutline();

    bool isSorted() const;

    OverviewModel *m_model = nullptr; // Not owned

    CppEditorWidget *m_editorWidget = nullptr;

    Utils::TreeViewComboBox *m_combo = nullptr; // Not owned
    QSortFilterProxyModel *m_proxyModel = nullptr;
    QAction *m_sortAction = nullptr;
    QTimer *m_updateIndexTimer = nullptr;
};

} // namespace Internal
} // namespace CppEditor
