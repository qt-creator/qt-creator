// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace ProjectExplorer { class Node; }

namespace qmt {
class MObject;
class MPackage;
class DiagramSceneController;
}

namespace ModelEditor {
namespace Internal {

class PxNodeUtilities :
        public QObject
{
    Q_OBJECT
    class PxNodeUtilitiesPrivate;

public:
    explicit PxNodeUtilities(QObject *parent = nullptr);
    ~PxNodeUtilities();

    void setDiagramSceneController(qmt::DiagramSceneController *diagramSceneController);

    QString calcRelativePath(const ProjectExplorer::Node *node, const QString &anchorFolder);
    QString calcRelativePath(const QString &filePath, const QString &anchorFolder);
    qmt::MPackage *createBestMatchingPackagePath(qmt::MPackage *suggestedParentPackage,
                                                 const QStringList &relativeElements);
    qmt::MObject *findSameObject(const QStringList &relativeElements,
                                 const qmt::MObject *object);
    bool isProxyHeader(const QString &file) const;

private:
    PxNodeUtilitiesPrivate *d;
};

} // namespace Internal
} // namespace ModelEditor
