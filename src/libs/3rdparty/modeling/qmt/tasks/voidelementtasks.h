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

class QMT_EXPORT VoidElementTasks : public IElementTasks
{
public:
    VoidElementTasks();
    ~VoidElementTasks() override;

    void openElement(const MElement *) override;
    void openElement(const DElement *, const MDiagram *) override;

    bool hasClassDefinition(const MElement *) const override;
    bool hasClassDefinition(const DElement *, const MDiagram *) const override;
    void openClassDefinition(const MElement *) override;
    void openClassDefinition(const DElement *, const MDiagram *) override;

    bool hasHeaderFile(const MElement *) const override;
    bool hasHeaderFile(const DElement *, const MDiagram *) const override;
    bool hasSourceFile(const MElement *) const override;
    bool hasSourceFile(const DElement *, const MDiagram *) const override;
    void openHeaderFile(const MElement *) override;
    void openHeaderFile(const DElement *, const MDiagram *) override;
    void openSourceFile(const MElement *) override;
    void openSourceFile(const DElement *, const MDiagram *) override;

    bool hasFolder(const MElement *) const override;
    bool hasFolder(const DElement *, const MDiagram *) const override;
    void showFolder(const MElement *) override;
    void showFolder(const DElement *, const MDiagram *) override;

    bool hasDiagram(const MElement *) const override;
    bool hasDiagram(const DElement *, const MDiagram *) const override;
    void openDiagram(const MElement *) override;
    void openDiagram(const DElement *, const MDiagram *) override;

    bool hasParentDiagram(const MElement *) const override;
    bool hasParentDiagram(const DElement *, const MDiagram *) const override;
    void openParentDiagram(const MElement *) override;
    void openParentDiagram(const DElement *, const MElement *) override;

    bool mayCreateDiagram(const qmt::MElement *) const override;
    bool mayCreateDiagram(const qmt::DElement *, const qmt::MDiagram *) const override;
    void createAndOpenDiagram(const qmt::MElement *) override;
    void createAndOpenDiagram(const qmt::DElement *, const qmt::MDiagram *) override;
};

} // namespace qmt

#endif // QMT_VOIDELEMENTTASKS_H
