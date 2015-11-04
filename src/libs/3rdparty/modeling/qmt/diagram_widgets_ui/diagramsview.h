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

#ifndef QMT_DIAGRAMSVIEW_H
#define QMT_DIAGRAMSVIEW_H

#include <QTabWidget>

#include "qmt/infrastructure/uid.h"
#include "qmt/diagram_ui/diagramsviewinterface.h"

#include <QPointer>
#include <QHash>


namespace qmt {

class MDiagram;

class DiagramView;
class DiagramSceneModel;
class DiagramsManager;


class QMT_EXPORT DiagramsView :
        public QTabWidget,
        public DiagramsViewInterface
{
    Q_OBJECT

public:
    explicit DiagramsView(QWidget *parent = 0);

    ~DiagramsView();

signals:

    void currentDiagramChanged(const MDiagram *diagram);

    void diagramCloseRequested(const MDiagram *diagram);

    void someDiagramOpened(bool);

public:

    void setDiagramsManager(DiagramsManager *diagramsManager);

public slots:

    void openDiagram(MDiagram *diagram);

    void closeDiagram(const MDiagram *diagram);

    void closeAllDiagrams();

    void onDiagramRenamed(const MDiagram *diagram);

private slots:

    void onCurrentChanged(int tabIndex);

    void onTabCloseRequested(int tabIndex);

private:

    MDiagram *diagram(int tabIndex) const;

    MDiagram *diagram(DiagramView * diagramView) const;

private:

    DiagramsManager *m_diagramsManager;

    QHash<Uid, DiagramView *> m_diagramViews;
};

}

#endif // DIAGRAMSVIEW_H
