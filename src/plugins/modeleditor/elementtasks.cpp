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

#include "elementtasks.h"

#include "modelsmanager.h"
#include "openelementvisitor.h"
#include "modeleditor_plugin.h"
#include "componentviewcontroller.h"

#include "qmt/diagram/delement.h"
#include "qmt/diagram/dpackage.h"
#include "qmt/document_controller/documentcontroller.h"
#include "qmt/infrastructure/contextmenuaction.h"
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

#include <QMenu>

namespace ModelEditor {
namespace Internal {

class ElementTasks::ElementTasksPrivate {
public:
    qmt::DocumentController *documentController = nullptr;
    ComponentViewController *componentViewController = nullptr;
};

ElementTasks::ElementTasks(QObject *parent)
    : QObject(parent),
      d(new ElementTasksPrivate)
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

void ElementTasks::setComponentViewController(ComponentViewController *componentViewController)
{
    d->componentViewController = componentViewController;
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
        QString qualifiedClassName = klass->umlNamespace().isEmpty()
                ? klass->name()
                : klass->umlNamespace() + "::" + klass->name();

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
        QString qualifiedClassName = klass->umlNamespace().isEmpty()
                ? klass->name()
                : klass->umlNamespace() + "::" + klass->name();

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
    return diagram != nullptr;
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
    while (element && element->owner()) {
        qmt::MObject *parentObject = element->owner()->owner();
        if (parentObject) {
            qmt::FindDiagramVisitor visitor;
            parentObject->accept(&visitor);
            const qmt::MDiagram *parentDiagram = visitor.diagram();
            if (parentDiagram) {
                return true;
            }
        }
        element = element->owner();
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
    while (element && element->owner()) {
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
        element = element->owner();
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
    return dynamic_cast<const qmt::MPackage *>(element) != nullptr;
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
            QMT_ASSERT(parentPackage, delete newDiagram; return);
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

bool ElementTasks::extendContextMenu(const qmt::DElement *delement, const qmt::MDiagram *, QMenu *menu)
{
    bool extended = false;
    if (dynamic_cast<const qmt::DPackage *>(delement)) {
        menu->addAction(new qmt::ContextMenuAction(tr("Update Include Dependencies"), "updateIncludeDependencies", menu));
        extended = true;
    }
    return extended;
}

bool ElementTasks::handleContextMenuAction(const qmt::DElement *element, const qmt::MDiagram *, const QString &id)
{
    if (id == "updateIncludeDependencies") {
        qmt::MPackage *mpackage = d->documentController->modelController()->findElement<qmt::MPackage>(element->modelUid());
        if (mpackage)
            d->componentViewController->updateIncludeDependencies(mpackage);
        return true;
    }
    return false;
}

} // namespace Internal
} // namespace ModelEditor
