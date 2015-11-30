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

#include "pxnodeutilities.h"

#include "qmt/controller/namecontroller.h"
#include "qmt/model/mpackage.h"
#include "qmt/tasks/diagramscenecontroller.h"
#include "qmt/model_controller/modelcontroller.h"

#include <projectexplorer/projectnodes.h>
#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QQueue>
#include <QPair>

#include <typeinfo>

namespace ModelEditor {
namespace Internal {

class PxNodeUtilities::PxNodeUtilitiesPrivate {
public:
    qmt::DiagramSceneController *diagramSceneController = 0;
};

PxNodeUtilities::PxNodeUtilities(QObject *parent)
    : QObject(parent),
      d(new PxNodeUtilitiesPrivate)
{
}

PxNodeUtilities::~PxNodeUtilities()
{
    delete d;
}

void PxNodeUtilities::setDiagramSceneController(qmt::DiagramSceneController *diagramSceneController)
{
    d->diagramSceneController = diagramSceneController;
}

QString PxNodeUtilities::calcRelativePath(const ProjectExplorer::Node *node,
                                          const QString &anchorFolder)
{
    QString nodePath;

    switch (node->nodeType()) {
    case ProjectExplorer::FileNodeType:
    {
        QFileInfo fileInfo = node->filePath().toFileInfo();
        nodePath = fileInfo.path();
        break;
    }
    case ProjectExplorer::FolderNodeType:
    case ProjectExplorer::VirtualFolderNodeType:
    case ProjectExplorer::ProjectNodeType:
        nodePath = node->filePath().toString();
        break;
    case ProjectExplorer::SessionNodeType:
        QTC_ASSERT(false, return QString());
        break;
    }

    return qmt::NameController::calcRelativePath(nodePath, anchorFolder);
}

qmt::MPackage *PxNodeUtilities::createBestMatchingPackagePath(
        qmt::MPackage *suggestedParentPackage, const QStringList &relativeElements)
{
    QSet<qmt::MPackage *> suggestedParents;
    qmt::MPackage *suggestedParent = suggestedParentPackage;
    while (suggestedParent) {
        suggestedParents.insert(suggestedParent);
        suggestedParent = dynamic_cast<qmt::MPackage *>(suggestedParent->owner());
    }

    QQueue<QPair<qmt::MPackage *, int> > roots;
    roots.append(qMakePair(d->diagramSceneController->modelController()->rootPackage(), 0));

    int maxChainLength = -1;
    int minChainDepth = -1;
    qmt::MPackage *bestParentPackage = 0;

    while (!roots.isEmpty()) {
        qmt::MPackage *package = roots.first().first;
        int depth = roots.first().second;
        roots.takeFirst();

        // append all sub-packages of the same level as next root packages
        foreach (const qmt::Handle<qmt::MObject> &handle, package->children()) {
            if (handle.hasTarget()) {
                if (auto childPackage = dynamic_cast<qmt::MPackage *>(handle.target())) {
                    // only accept root packages in the same path as the suggested parent package
                    if (suggestedParents.contains(childPackage)) {
                        roots.append(qMakePair(childPackage, depth + 1));
                        break;
                    }
                }
            }
        }

        // goto into sub-packages to find chain of names
        int relativeIndex = 0;
        bool found = true;
        while (found && relativeIndex < relativeElements.size()) {
            QString relativeSearchId = qmt::NameController::calcElementNameSearchId(
                        relativeElements.at(relativeIndex));
            found = false;
            foreach (const qmt::Handle<qmt::MObject> &handle, package->children()) {
                if (handle.hasTarget()) {
                    if (auto childPackage = dynamic_cast<qmt::MPackage *>(handle.target())) {
                        if (qmt::NameController::calcElementNameSearchId(childPackage->name()) == relativeSearchId) {
                            package = childPackage;
                            ++relativeIndex;
                            found = true;
                            break;
                        }
                    }
                }
            }
        }

        if (found)
            return package; // complete chain found, innermost package is already the result

        QTC_CHECK(!(relativeIndex == maxChainLength && minChainDepth < 0));
        if (relativeIndex >= 1
                && (relativeIndex > maxChainLength
                    || (relativeIndex == maxChainLength && depth < minChainDepth))) {
            maxChainLength = relativeIndex;
            minChainDepth = depth;
            bestParentPackage = package;
        }
    }

    QTC_CHECK(maxChainLength < relativeElements.size());
    if (!bestParentPackage) {
        QTC_CHECK(maxChainLength == -1);
        QTC_CHECK(minChainDepth == -1);
        maxChainLength = 0;
        bestParentPackage = suggestedParentPackage;
    } else {
        QTC_CHECK(maxChainLength >= 1);
    }

    int i = maxChainLength;
    while (i < relativeElements.size()) {
        auto newPackage = new qmt::MPackage();
        newPackage->setFlags(qmt::MElement::ReverseEngineered);
        newPackage->setName(relativeElements.at(i));
        d->diagramSceneController->modelController()->addObject(bestParentPackage, newPackage);
        bestParentPackage = newPackage;
        ++i;
    }
    return bestParentPackage;
}

qmt::MObject *PxNodeUtilities::findSameObject(const QStringList &relativeElements,
                                              const qmt::MObject *object)
{
    QQueue<qmt::MPackage *> roots;
    roots.append(d->diagramSceneController->modelController()->rootPackage());

    while (!roots.isEmpty()) {
        qmt::MPackage *package = roots.takeFirst();

        // append all sub-packages of the same level as next root packages
        foreach (const qmt::Handle<qmt::MObject> &handle, package->children()) {
            if (handle.hasTarget()) {
                if (auto childPackage = dynamic_cast<qmt::MPackage *>(handle.target()))
                    roots.append(childPackage);
            }
        }

        // goto into sub-packages to find complete chain of names
        int relativeIndex = 0;
        bool found = true;
        while (found && relativeIndex < relativeElements.size()) {
            QString relativeSearchId = qmt::NameController::calcElementNameSearchId(
                        relativeElements.at(relativeIndex));
            found = false;
            foreach (const qmt::Handle<qmt::MObject> &handle, package->children()) {
                if (handle.hasTarget()) {
                    if (auto childPackage = dynamic_cast<qmt::MPackage *>(handle.target())) {
                        if (qmt::NameController::calcElementNameSearchId(childPackage->name()) == relativeSearchId) {
                            package = childPackage;
                            ++relativeIndex;
                            found = true;
                            break;
                        }
                    }
                }
            }
        }

        if (found) {
            QTC_CHECK(relativeIndex >= relativeElements.size());
            // chain was found so check for given object within deepest package
            QString objectSearchId = qmt::NameController::calcElementNameSearchId(object->name());
            foreach (const qmt::Handle<qmt::MObject> &handle, package->children()) {
                if (handle.hasTarget()) {
                    qmt::MObject *target = handle.target();
                    if (typeid(*target) == typeid(*object)
                            && qmt::NameController::calcElementNameSearchId(target->name()) == objectSearchId) {
                        return target;
                    }
                }
            }
        }
    }

    // complete sub-package structure scanned but did not found the desired object
    return 0;
}

} // namespace Internal
} // namespace ModelEditor
