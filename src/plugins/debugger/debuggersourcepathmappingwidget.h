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

#include <QGroupBox>
#include <QMap>

QT_BEGIN_NAMESPACE
class QStandardItemModel;
class QTreeView;
class QLineEdit;
class QPushButton;
class QLineEdit;
class QModelIndex;
QT_END_NAMESPACE

namespace Utils { class PathChooser; }

namespace Debugger {
namespace Internal {

class DebuggerRunParameters;
class SourcePathMappingModel;

class DebuggerSourcePathMappingWidget : public QGroupBox
{
    Q_OBJECT

public:
    typedef QMap<QString, QString> SourcePathMap;

    explicit DebuggerSourcePathMappingWidget(QWidget *parent = 0);

    SourcePathMap sourcePathMap() const;
    void setSourcePathMap(const SourcePathMap &);

    /* Merge settings for an installed Qt (unless another setting
     * is already in the map. */
    static SourcePathMap mergePlatformQtPath(const DebuggerRunParameters &sp,
                                             const SourcePathMap &in);

private:
    void slotAdd();
    void slotAddQt();
    void slotRemove();
    void slotCurrentRowChanged(const QModelIndex &,const QModelIndex &);
    void slotEditSourceFieldChanged();
    void slotEditTargetFieldChanged();

    void resizeColumns();
    void updateEnabled();
    QString editSourceField() const;
    QString editTargetField() const;
    void setEditFieldMapping(const QPair<QString, QString> &m);
    int currentRow() const;
    void setCurrentRow(int r);

    SourcePathMappingModel *m_model;
    QTreeView *m_treeView;
    QPushButton *m_addButton;
    QPushButton *m_addQtButton;
    QPushButton *m_removeButton;
    QLineEdit *m_sourceLineEdit;
    Utils::PathChooser *m_targetChooser;
};

} // namespace Internal
} // namespace Debugger
