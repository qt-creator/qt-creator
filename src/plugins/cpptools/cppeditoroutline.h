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

#ifndef CPPEDITOROUTLINE_H
#define CPPEDITOROUTLINE_H

#include "cpptools_global.h"

#include <QModelIndex>
#include <QObject>

QT_BEGIN_NAMESPACE
class QAction;
class QSortFilterProxyModel;
class QTimer;
QT_END_NAMESPACE

namespace CPlusPlus { class OverviewModel; }
namespace TextEditor { class TextEditorWidget; }
namespace Utils { class TreeViewComboBox; }

namespace CppTools {

class CPPTOOLS_EXPORT CppEditorOutline : public QObject
{
    Q_OBJECT

public:
    explicit CppEditorOutline(TextEditor::TextEditorWidget *editorWidget);

    void update();

    CPlusPlus::OverviewModel *model() const;
    QModelIndex modelIndex();

    QWidget *widget() const; // Must be deleted by client.

signals:
    void modelIndexChanged(const QModelIndex &index);

public slots:
    void updateIndex();
    void setSorted(bool sort);

private slots:
    void updateNow();
    void updateIndexNow();
    void updateToolTip();
    void gotoSymbolInEditor();

private:
    CppEditorOutline();

    bool isSorted() const;
    QModelIndex indexForPosition(int line, int column,
                                 const QModelIndex &rootIndex = QModelIndex()) const;

private:
    TextEditor::TextEditorWidget *m_editorWidget;

    Utils::TreeViewComboBox *m_combo; // Not owned
    CPlusPlus::OverviewModel *m_model;
    QSortFilterProxyModel *m_proxyModel;
    QModelIndex m_modelIndex;
    QAction *m_sortAction;
    QTimer *m_updateTimer;
    QTimer *m_updateIndexTimer;
};

} // namespace CppTools

#endif // CPPEDITOROUTLINE_H
