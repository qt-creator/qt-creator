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

#ifndef DIAGRAMSVIEWMANAGER_H
#define DIAGRAMSVIEWMANAGER_H

#include <QObject>
#include "qmt/diagram_ui/diagramsviewinterface.h"

#include <QHash>

namespace qmt {
class Uid;
class DiagramView;
class MDiagram;
}

namespace ModelEditor {
namespace Internal {

class DiagramsViewManager :
        public QObject,
        public qmt::DiagramsViewInterface
{
    Q_OBJECT

public:
    explicit DiagramsViewManager(QObject *parent = 0);
    ~DiagramsViewManager() = default;

signals:
    void openNewDiagram(qmt::MDiagram *diagram);
    void closeOpenDiagram(const qmt::MDiagram *diagram);
    void closeAllOpenDiagrams();
    void diagramRenamed(const qmt::MDiagram *diagram);

public:
    void openDiagram(qmt::MDiagram *diagram) override;
    void closeDiagram(const qmt::MDiagram *diagram) override;
    void closeAllDiagrams() override;
    void onDiagramRenamed(const qmt::MDiagram *diagram) override;
};

} // namespace Internal
} // namespace ModelEditor

#endif // DIAGRAMSVIEWMANAGER_H
