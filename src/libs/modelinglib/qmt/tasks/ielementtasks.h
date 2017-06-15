/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE
class QMenu;
class QString;
QT_END_NAMESPACE

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

    virtual bool hasParentDiagram(const MElement *) const = 0;
    virtual bool hasParentDiagram(const DElement *, const MDiagram *) const = 0;
    virtual void openParentDiagram(const MElement *) = 0;
    virtual void openParentDiagram(const DElement *, const MElement *) = 0;

    virtual bool mayCreateDiagram(const MElement *) const = 0;
    virtual bool mayCreateDiagram(const DElement *, const MDiagram *) const = 0;
    virtual void createAndOpenDiagram(const MElement *) = 0;
    virtual void createAndOpenDiagram(const DElement *, const MDiagram *) = 0;

    virtual bool extendContextMenu(const DElement *, const MDiagram *, QMenu *) = 0;
    virtual bool handleContextMenuAction(const DElement *, const MDiagram *, const QString &) = 0;
};

} // namespace qmt
