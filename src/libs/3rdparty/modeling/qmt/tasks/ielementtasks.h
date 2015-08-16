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

#ifndef QMT_IELEMENTTASKS_H
#define QMT_IELEMENTTASKS_H

namespace qmt {

class MElement;
class DElement;
class MDiagram;

class IElementTasks
{
public:

    virtual ~IElementTasks() { }

    virtual void openElement(const MElement *) = 0;
    virtual void openElement(const DElement *, const MDiagram *) = 0;

    virtual bool hasClassDefinition(const MElement *) const = 0;
    virtual bool hasClassDefinition(const DElement *, const MDiagram *) const = 0;
    virtual void openClassDefinition(const MElement *) = 0;
    virtual void openClassDefinition(const DElement *, const MDiagram *) = 0;

    virtual bool hasHeaderFile(const MElement *) const = 0;
    virtual bool hasHeaderFile(const DElement *, const MDiagram *) const = 0;
    virtual bool hasSourceFile(const MElement *) const = 0;
    virtual bool hasSourceFile(const DElement *, const MDiagram *) const = 0;
    virtual void openHeaderFile(const MElement *) = 0;
    virtual void openHeaderFile(const DElement *, const MDiagram *) = 0;
    virtual void openSourceFile(const MElement *) = 0;
    virtual void openSourceFile(const DElement *, const MDiagram *) = 0;

    virtual bool hasFolder(const MElement *) const = 0;
    virtual bool hasFolder(const DElement *, const MDiagram *) const = 0;
    virtual void showFolder(const MElement *) = 0;
    virtual void showFolder(const DElement *, const MDiagram *) = 0;

    virtual bool hasDiagram(const MElement *) const = 0;
    virtual bool hasDiagram(const DElement *, const MDiagram *) const = 0;
    virtual void openDiagram(const MElement *) = 0;
    virtual void openDiagram(const DElement *, const MDiagram *) = 0;

    virtual bool mayCreateDiagram(const MElement *) const = 0;
    virtual bool mayCreateDiagram(const DElement *, const MDiagram *) const = 0;
    virtual void createAndOpenDiagram(const MElement *) = 0;
    virtual void createAndOpenDiagram(const DElement *, const MDiagram *) = 0;
};

}

#endif // QMT_IELEMENTTASKS_H
