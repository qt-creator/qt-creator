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

#include "elementtasks.h"

#include "modelsmanager.h"
#include "openelementvisitor.h"
#include "modeleditor_plugin.h"

#include "qmt/diagram/delement.h"
#include "qmt/document_controller/documentcontroller.h"
#include "qmt/model/melement.h"
#include "qmt/model/mclass.h"
#include "qmt/model/mdiagram.h"
#include "qmt/model/mcanvasdiagram.h"
#include "qmt/model/mpackage.h"
#include "qmt/model_controller/modelcontroller.h"
#include "qmt/tasks/finddiagramvisitor.h"
#include "qmt/project_controller/projectcontroller.h"
#include "qmt/project/project.h"

#include <extensionsystem/pluginmanager.h>
#include <cpptools/cppclassesfilter.h>
#include <cpptools/indexitem.h>
#include <cpptools/searchsymbols.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/locator/ilocatorfilter.h>
#include <utils/qtcassert.h>

namespace ModelEditor {
namespace Internal {

class ElementTasks::ElementTasksPrivate {
public:
    qmt::DocumentController *documentController = 0;
};

ElementTasks::ElementTasks()
    : d(new ElementTasksPrivate)
{
}

ElementTasks::~ElementTasks()
{
    delete d;
}

void ElementTasks::setDocumentController(qmt::DocumentController *documentController)
{
    d->documentController = documentController;
}

void ElementTasks::openElement(const qmt::MElement *element)
{
    OpenModelElementVisitor visitor;
    visitor.setElementTasks(this);
    element->accept(&visitor);
}

void ElementTasks::openElement(const qmt::DElement *element, const qmt::MDiagram *diagram)
{
    Q_UNUSED(diagram);

    OpenDiagramElementVisitor visitor;
    visitor.setModelController(d->documentController->modelController());
    visitor.setElementTasks(this);
    element->accept(&visitor);
}

bool ElementTasks::hasClassDefinition(const qmt::MElement *element) const
{
    if (auto klass = dynamic_cast<const qmt::MClass *>(element)) {
        QString qualifiedClassName = klass->nameSpace().isEmpty()
                ? klass->name()
                : klass->nameSpace() + QStringLiteral("::") + klass->name();

        CppTools::CppClassesFilter *classesFilter = ExtensionSystem::PluginManager::getObject<CppTools::CppClassesFilter>();
        if (!classesFilter)
            return false;

        QFutureInterface<Core::LocatorFilterEntry> dummyInterface;
        QList<Core::LocatorFilterEntry> matches = classesFilter->matchesFor(dummyInterface,
                                                                            qualifiedClassName);
        foreach (const Core::LocatorFilterEntry &entry, matches) {
            CppTools::IndexItem::Ptr info = qvariant_cast<CppTools::IndexItem::Ptr>(entry.internalData);
            if (info->scopedSymbolName() != qualifiedClassName)
                continue;
            return true;
        }
    }
    return false;
}

bool ElementTasks::hasClassDefinition(const qmt::DElement *element,
                                      const qmt::MDiagram *diagram) const
{
    Q_UNUSED(diagram);

    qmt::MElement *melement = d->documentController->modelController()->findElement(
                element->modelUid());
    if (!melement)
        return false;
    return hasClassDefinition(melement);
}

void ElementTasks::openClassDefinition(const qmt::MElement *element)
{
    if (auto klass = dynamic_cast<const qmt::MClass *>(element)) {
        QString qualifiedClassName = klass->nameSpace().isEmpty()
                ? klass->name()
                : klass->nameSpace() + QStringLiteral("::") + klass->name();

        CppTools::CppClassesFilter *classesFilter = ExtensionSystem::PluginManager::getObject<CppTools::CppClassesFilter>();
        if (!classesFilter)
            return;

        QFutureInterface<Core::LocatorFilterEntry> dummyInterface;
        QList<Core::LocatorFilterEntry> matches = classesFilter->matchesFor(dummyInterface, qualifiedClassName);
        foreach (const Core::LocatorFilterEntry &entry, matches) {
            CppTools::IndexItem::Ptr info = qvariant_cast<CppTools::IndexItem::Ptr>(entry.internalData);
            if (info->scopedSymbolName() != qualifiedClassName)
                continue;
            if (Core::EditorManager::instance()->openEditorAt(info->fileName(), info->line(), info->column()))
                return;
        }
    }
}

void ElementTasks::openClassDefinition(const qmt::DElement *element, const qmt::MDiagram *diagram)
{
    Q_UNUSED(diagram);

    qmt::MElement *melement = d->documentController->modelController()->findElement(element->modelUid());
    if (!melement)
        return;
    openClassDefinition(melement);
}

bool ElementTasks::hasHeaderFile(const qmt::MElement *element) const
{
    // TODO implement
    Q_UNUSED(element);
    return false;
}

bool ElementTasks::hasHeaderFile(const qmt::DElement *element, const qmt::MDiagram *diagram) const
{
    Q_UNUSED(diagram);

    qmt::MElement *melement = d->documentController->modelController()->findElement(element->modelUid());
    if (!melement)
        return false;
    return hasHeaderFile(melement);
}

bool ElementTasks::hasSourceFile(const qmt::MElement *element) const
{
    // TODO implement
    Q_UNUSED(element);
    return false;
}

bool ElementTasks::hasSourceFile(const qmt::DElement *element, const qmt::MDiagram *diagram) const
{
    Q_UNUSED(diagram);

    qmt::MElement *melement = d->documentController->modelController()->findElement(element->modelUid());
    if (!melement)
        return false;
    return hasSourceFile(melement);
}

void ElementTasks::openHeaderFile(const qmt::MElement *element)
{
    // TODO implement
    Q_UNUSED(element);
}

void ElementTasks::openHeaderFile(const qmt::DElement *element, const qmt::MDiagram *diagram)
{
    Q_UNUSED(diagram);

    qmt::MElement *melement = d->documentController->modelController()->findElement(element->modelUid());
    if (!melement)
        return;
    openHeaderFile(melement);
}

void ElementTasks::openSourceFile(const qmt::MElement *element)
{
    // TODO implement
    Q_UNUSED(element);
}

void ElementTasks::openSourceFile(const qmt::DElement *element, const qmt::MDiagram *diagram)
{
    Q_UNUSED(diagram);

    qmt::MElement *melement = d->documentController->modelController()->findElement(element->modelUid());
    if (!melement)
        return;
    openSourceFile(melement);
}

bool ElementTasks::hasFolder(const qmt::MElement *element) const
{
    // TODO implement
    Q_UNUSED(element);
    return false;
}

bool ElementTasks::hasFolder(const qmt::DElement *element, const qmt::MDiagram *diagram) const
{
    Q_UNUSED(diagram);

    qmt::MElement *melement = d->documentController->modelController()->findElement(element->modelUid());
    if (!melement)
        return false;
    return hasFolder(melement);
}

void ElementTasks::showFolder(const qmt::MElement *element)
{
    // TODO implement
    Q_UNUSED(element);
}

void ElementTasks::showFolder(const qmt::DElement *element, const qmt::MDiagram *diagram)
{
    Q_UNUSED(diagram);

    qmt::MElement *melement = d->documentController->modelController()->findElement(element->modelUid());
    if (!melement)
        return;
    showFolder(melement);
}

bool ElementTasks::hasDiagram(const qmt::MElement *element) const
{
    qmt::FindDiagramVisitor visitor;
    element->accept(&visitor);
    const qmt::MDiagram *diagram = visitor.diagram();
    return diagram != 0;
}

bool ElementTasks::hasDiagram(const qmt::DElement *element, const qmt::MDiagram *diagram) const
{
    Q_UNUSED(diagram);

    qmt::MElement *melement = d->documentController->modelController()->findElement(element->modelUid());
    if (!melement)
        return false;
    return hasDiagram(melement);
}

void ElementTasks::openDiagram(const qmt::MElement *element)
{
    qmt::FindDiagramVisitor visitor;
    element->accept(&visitor);
    const qmt::MDiagram *diagram = visitor.diagram();
    if (diagram) {
        ModelEditorPlugin::modelsManager()->openDiagram(
                    d->documentController->projectController()->project()->uid(),
                    diagram->uid());
    }
}

void ElementTasks::openDiagram(const qmt::DElement *element, const qmt::MDiagram *diagram)
{
    Q_UNUSED(diagram);

    qmt::MElement *melement = d->documentController->modelController()->findElement(element->modelUid());
    if (!melement)
        return;
    openDiagram(melement);
}

bool ElementTasks::hasParentDiagram(const qmt::MElement *element) const
{
    if (element && element->owner()) {
        qmt::MObject *parentObject = element->owner()->owner();
        if (parentObject) {
            qmt::FindDiagramVisitor visitor;
            parentObject->accept(&visitor);
            const qmt::MDiagram *parentDiagram = visitor.diagram();
            if (parentDiagram) {
                return true;
            }
        }
    }
    return false;
}

bool ElementTasks::hasParentDiagram(const qmt::DElement *element, const qmt::MDiagram *diagram) const
{
    Q_UNUSED(diagram);

    if (!element)
        return false;

    qmt::MElement *melement = d->documentController->modelController()->findElement(element->modelUid());
    if (!melement)
        return false;
    return hasParentDiagram(melement);
}

void ElementTasks::openParentDiagram(const qmt::MElement *element)
{
    if (element && element->owner()) {
        qmt::MObject *parentObject = element->owner()->owner();
        if (parentObject) {
            qmt::FindDiagramVisitor visitor;
            parentObject->accept(&visitor);
            const qmt::MDiagram *parentDiagram = visitor.diagram();
            if (parentDiagram) {
                ModelEditorPlugin::modelsManager()->openDiagram(
                            d->documentController->projectController()->project()->uid(),
                            parentDiagram->uid());
                return;
            }
        }
    }
}

void ElementTasks::openParentDiagram(const qmt::DElement *element, const qmt::MElement *diagram)
{
    Q_UNUSED(diagram);

    if (!element)
        return;

    qmt::MElement *melement = d->documentController->modelController()->findElement(element->modelUid());
    if (!melement)
        return;
    openParentDiagram(melement);
}

bool ElementTasks::mayCreateDiagram(const qmt::MElement *element) const
{
    return dynamic_cast<const qmt::MPackage *>(element) != 0;
}

bool ElementTasks::mayCreateDiagram(const qmt::DElement *element,
                                    const qmt::MDiagram *diagram) const
{
    Q_UNUSED(diagram);

    qmt::MElement *melement = d->documentController->modelController()->findElement(element->modelUid());
    if (!melement)
        return false;
    return mayCreateDiagram(melement);
}

void ElementTasks::createAndOpenDiagram(const qmt::MElement *element)
{
    if (auto package = dynamic_cast<const qmt::MPackage *>(element)) {
        qmt::FindDiagramVisitor visitor;
        element->accept(&visitor);
        const qmt::MDiagram *diagram = visitor.diagram();
        if (diagram) {
            ModelEditorPlugin::modelsManager()->openDiagram(
                        d->documentController->projectController()->project()->uid(),
                        diagram->uid());
        } else {
            auto newDiagram = new qmt::MCanvasDiagram();
            newDiagram->setName(package->name());
            qmt::MPackage *parentPackage = d->documentController->modelController()->findObject<qmt::MPackage>(package->uid());
            QTC_ASSERT(parentPackage, delete newDiagram; return);
            d->documentController->modelController()->addObject(parentPackage, newDiagram);
            ModelEditorPlugin::modelsManager()->openDiagram(
                        d->documentController->projectController()->project()->uid(),
                        newDiagram->uid());
        }
    }
}

void ElementTasks::createAndOpenDiagram(const qmt::DElement *element, const qmt::MDiagram *diagram)
{
    Q_UNUSED(diagram);

    qmt::MElement *melement = d->documentController->modelController()->findElement(element->modelUid());
    if (!melement)
        return;
    createAndOpenDiagram(melement);
}

} // namespace Internal
} // namespace ModelEditor
