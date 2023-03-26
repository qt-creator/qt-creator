// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
