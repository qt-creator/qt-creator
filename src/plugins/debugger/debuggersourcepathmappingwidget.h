/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef DEBUGGERSOURCEPATHMAPPINGWIDGET_H
#define DEBUGGERSOURCEPATHMAPPINGWIDGET_H

#include <QtGui/QGroupBox>
#include <QtCore/QMap>
#include <QtCore/QPair>

QT_BEGIN_NAMESPACE
class QStandardItemModel;
class QTreeView;
class QLineEdit;
class QPushButton;
class QLineEdit;
class QModelIndex;
QT_END_NAMESPACE

namespace Utils {
class PathChooser;
}

namespace Debugger {
namespace Internal {
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
    static SourcePathMap mergePlatformQtPath(const QString &qtInstallPath,
                                             const SourcePathMap &in);

signals:

private slots:
    void slotAdd();
    void slotAddQt();
    void slotRemove();
    void slotCurrentRowChanged(const QModelIndex &,const QModelIndex &);
    void slotEditSourceFieldChanged();
    void slotEditTargetFieldChanged();

private:
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

#endif // DEBUGGERSOURCEPATHMAPPINGWIDGET_H
