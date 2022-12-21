// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmldesigner_global.h"

#include <QObject>
#include <QList>
#include <QLoggingCategory>

#include <designdocument.h>

#include <map>
#include <memory>

namespace Core { class IEditor; }
namespace ProjectExplorer { class Node; }
namespace ProjectExplorer { class Project; }
namespace QmlDesigner {

class QmlDesignerProjectManager;

Q_DECLARE_LOGGING_CATEGORY(documentManagerLog)

class QMLDESIGNER_EXPORT DocumentManager : public QObject
{
    Q_OBJECT
public:
    DocumentManager(QmlDesignerProjectManager &projectManager,
                    ExternalDependenciesInterface &externalDependencies)
        : m_projectManager{projectManager}
        , m_externalDependencies{externalDependencies}
    {}

    void setCurrentDesignDocument(Core::IEditor *editor);
    DesignDocument *currentDesignDocument() const;
    bool hasCurrentDesignDocument() const;

    void removeEditors(const QList<Core::IEditor *> &editors);

    void resetPossibleImports();

    static bool goIntoComponent(const ModelNode &modelNode);
    static void goIntoComponent(const QString &fileName);

    static bool createFile(const QString &filePath, const QString &contents);
    static void addFileToVersionControl(const QString &directoryPath, const QString &newFilePath);
    static Utils::FilePath currentFilePath();
    static Utils::FilePath currentProjectDirPath();

    static QStringList isoIconsQmakeVariableValue(const QString &proPath);
    static bool setIsoIconsQmakeVariableValue(const QString &proPath, const QStringList &value);
    static void findPathToIsoProFile(bool *iconResourceFileAlreadyExists, QString *resourceFilePath,
        QString *resourceFileProPath, const QString &isoIconsQrcFile);
    static bool isoProFileSupportsAddingExistingFiles(const QString &resourceFileProPath);
    static bool addResourceFileToIsoProject(const QString &resourceFileProPath, const QString &resourceFilePath);
    static bool belongsToQmakeProject();
    static Utils::FilePath currentResourcePath();

private:
    std::map<Core::IEditor *, std::unique_ptr<DesignDocument>> m_designDocuments;
    QPointer<DesignDocument> m_currentDesignDocument;
    QmlDesignerProjectManager &m_projectManager;
    ExternalDependenciesInterface &m_externalDependencies;
};

} // namespace QmlDesigner
