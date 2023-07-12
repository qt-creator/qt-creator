// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pxnodeutilities.h"

#include "qmt/controller/namecontroller.h"
#include "qmt/model/mpackage.h"
#include "qmt/tasks/diagramscenecontroller.h"
#include "qmt/model_controller/modelcontroller.h"

#include <cppeditor/cppmodelmanager.h>
#include <cplusplus/CppDocument.h>

#include <projectexplorer/projectnodes.h>
#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QList>
#include <QPair>

#include <typeinfo>

namespace ModelEditor {
namespace Internal {

class PxNodeUtilities::PxNodeUtilitiesPrivate {
public:
    qmt::DiagramSceneController *diagramSceneController = nullptr;
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
    const QString nodePath = node->asFileNode()
            ? node->filePath().toFileInfo().path()
            : node->filePath().toString();

    return qmt::NameController::calcRelativePath(nodePath, anchorFolder);
}

QString PxNodeUtilities::calcRelativePath(const QString &filePath, const QString &anchorFolder)
{
    QString path;

    QFileInfo fileInfo(filePath);
    if (fileInfo.exists() && fileInfo.isFile())
        path = fileInfo.path();
    else
        path = filePath;
    return qmt::NameController::calcRelativePath(path, anchorFolder);
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

    QList<QPair<qmt::MPackage *, int>>
            roots{{d->diagramSceneController->modelController()->rootPackage(), 0}};

    int maxChainLength = -1;
    int minChainDepth = -1;
    qmt::MPackage *bestParentPackage = nullptr;

    while (!roots.isEmpty()) {
        qmt::MPackage *package = roots.first().first;
        int depth = roots.first().second;
        roots.takeFirst();

        // append all sub-packages of the same level as next root packages
        for (const qmt::Handle<qmt::MObject> &handle : package->children()) {
            if (handle.hasTarget()) {
                if (auto childPackage = dynamic_cast<qmt::MPackage *>(handle.target())) {
                    // only accept root packages in the same path as the suggested parent package
                    if (suggestedParents.contains(childPackage)) {
                        roots.push_back({childPackage, depth + 1});
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
            for (const qmt::Handle<qmt::MObject> &handle : package->children()) {
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

        QMT_CHECK(!(relativeIndex == maxChainLength && minChainDepth < 0));
        if (relativeIndex >= 1
                && (relativeIndex > maxChainLength
                    || (relativeIndex == maxChainLength && depth < minChainDepth))) {
            maxChainLength = relativeIndex;
            minChainDepth = depth;
            bestParentPackage = package;
        }
    }

    QMT_CHECK(maxChainLength < relativeElements.size());
    if (!bestParentPackage) {
        QMT_CHECK(maxChainLength == -1);
        QMT_CHECK(minChainDepth == -1);
        maxChainLength = 0;
        bestParentPackage = suggestedParentPackage;
    } else {
        QMT_CHECK(maxChainLength >= 1);
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
    QList<qmt::MPackage *> roots{d->diagramSceneController->modelController()->rootPackage()};

    while (!roots.isEmpty()) {
        qmt::MPackage *package = roots.takeFirst();

        // append all sub-packages of the same level as next root packages
        for (const qmt::Handle<qmt::MObject> &handle : package->children()) {
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
            for (const qmt::Handle<qmt::MObject> &handle : package->children()) {
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
            QMT_CHECK(relativeIndex >= relativeElements.size());
            // chain was found so check for given object within deepest package
            QString objectSearchId = qmt::NameController::calcElementNameSearchId(object->name());
            for (const qmt::Handle<qmt::MObject> &handle : package->children()) {
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
    return nullptr;
}

bool PxNodeUtilities::isProxyHeader(const QString &file) const
{
    CPlusPlus::Snapshot snapshot = CppEditor::CppModelManager::snapshot();

    CPlusPlus::Document::Ptr document = snapshot.document(Utils::FilePath::fromString(file));
    if (document) {
        QList<CPlusPlus::Document::Include> includes = document->resolvedIncludes();
        if (includes.count() != 1)
            return false;
        return includes.at(0).resolvedFileName().fileName() == QFileInfo(file).fileName();
    }
    return false;
}

} // namespace Internal
} // namespace ModelEditor
