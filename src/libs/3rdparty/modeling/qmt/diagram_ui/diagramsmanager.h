/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
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

#ifndef QMT_DIAGRAMSMANAGER_H
#define QMT_DIAGRAMSMANAGER_H

#include <QObject>

#include "qmt/infrastructure/uid.h"

#include <QHash>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

namespace qmt {

class MDiagram;
class DiagramController;
class DiagramSceneController;
class StyleController;
class StereotypeController;
class DSelection;
class DElement;

class TreeModel;
class DiagramsViewInterface;
class DiagramSceneModel;

class QMT_EXPORT DiagramsManager : public QObject
{
    Q_OBJECT
    class ManagedDiagram;

public:
    explicit DiagramsManager(QObject *parent = 0);
    ~DiagramsManager();

signals:
    void someDiagramOpened(bool);
    void diagramActivated(const MDiagram *diagram);
    void diagramSelectionChanged(const MDiagram *diagram);

public:
    void setModel(TreeModel *model);
    void setDiagramsView(DiagramsViewInterface *diagramsView);
    void setDiagramController(DiagramController *diagramController);
    void setDiagramSceneController(DiagramSceneController *diagramSceneController);
    void setStyleController(StyleController *styleController);
    void setStereotypeController(StereotypeController *stereotypeController);

    DiagramSceneModel *bindDiagramSceneModel(MDiagram *diagram);
    DiagramSceneModel *diagramSceneModel(const MDiagram *diagram) const;
    void unbindDiagramSceneModel(const MDiagram *diagram);

public slots:
    void openDiagram(MDiagram *diagram);
    void removeDiagram(const MDiagram *diagram);
    void removeAllDiagrams();

private slots:
    void onDataChanged(const QModelIndex &topleft, const QModelIndex &bottomright);

private:
    QPointer<TreeModel> m_model;
    DiagramsViewInterface *m_diagramsView;
    DiagramController *m_diagramController;
    DiagramSceneController *m_diagramSceneController;
    StyleController *m_styleController;
    StereotypeController *m_stereotypeController;
    QHash<Uid, ManagedDiagram *> m_diagramUidToManagedDiagramMap;
};

} // namespace qmt

#endif // QMT_DIAGRAMSMANAGER_H
