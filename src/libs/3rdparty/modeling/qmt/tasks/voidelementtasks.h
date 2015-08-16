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

#ifndef QMT_VOIDELEMENTTASKS_H
#define QMT_VOIDELEMENTTASKS_H

#include "ielementtasks.h"
#include "qmt/infrastructure/qmt_global.h"

namespace qmt {

class QMT_EXPORT VoidElementTasks :
        public IElementTasks
{
public:
    VoidElementTasks();
    ~VoidElementTasks();

public:

    void openElement(const MElement *);
    void openElement(const DElement *, const MDiagram *);

    bool hasClassDefinition(const MElement *) const;
    bool hasClassDefinition(const DElement *, const MDiagram *) const;
    void openClassDefinition(const MElement *);
    void openClassDefinition(const DElement *, const MDiagram *);

    bool hasHeaderFile(const MElement *) const;
    bool hasHeaderFile(const DElement *, const MDiagram *) const;
    bool hasSourceFile(const MElement *) const;
    bool hasSourceFile(const DElement *, const MDiagram *) const;
    void openHeaderFile(const MElement *);
    void openHeaderFile(const DElement *, const MDiagram *);
    void openSourceFile(const MElement *);
    void openSourceFile(const DElement *, const MDiagram *);

    bool hasFolder(const MElement *) const;
    bool hasFolder(const DElement *, const MDiagram *) const;
    void showFolder(const MElement *);
    void showFolder(const DElement *, const MDiagram *);

    bool hasDiagram(const MElement *) const;
    bool hasDiagram(const DElement *, const MDiagram *) const;
    void openDiagram(const MElement *);
    void openDiagram(const DElement *, const MDiagram *);

    bool mayCreateDiagram(const qmt::MElement *) const;
    bool mayCreateDiagram(const qmt::DElement *, const qmt::MDiagram *) const;
    void createAndOpenDiagram(const qmt::MElement *);
    void createAndOpenDiagram(const qmt::DElement *, const qmt::MDiagram *);
};

}

#endif // QMT_VOIDELEMENTTASKS_H
