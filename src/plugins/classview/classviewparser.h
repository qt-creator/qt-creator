// Copyright (C) 2016 Denis Mingulov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

#include "classviewparsertreeitem.h"

namespace ClassView {
namespace Internal {

class ParserPrivate;

class Parser : public QObject
{
    Q_OBJECT

public:
    explicit Parser(QObject *parent = nullptr);
    ~Parser() override;

    void requestCurrentState();
    void removeFiles(const QStringList &fileList);
    void resetData(const QHash<Utils::FilePath, QPair<QString, Utils::FilePaths>> &projects);
    void addProject(const Utils::FilePath &projectPath, const QString &projectName,
                    const Utils::FilePaths &filesInProject);
    void removeProject(const Utils::FilePath &projectPath);
    void setFlatMode(bool flat);

    void updateDocuments(const QSet<Utils::FilePath> &documentPaths);

signals:
    void treeRegenerated(const ParserTreeItem::ConstPtr &root);

private:
    void updateDocumentsFromSnapshot(const QSet<Utils::FilePath> &documentPaths,
                                     const CPlusPlus::Snapshot &snapshot);

    ParserTreeItem::ConstPtr getParseDocumentTree(const CPlusPlus::Document::Ptr &doc);
    ParserTreeItem::ConstPtr getCachedOrParseDocumentTree(const CPlusPlus::Document::Ptr &doc);
    ParserTreeItem::ConstPtr getParseProjectTree(const Utils::FilePath &projectPath,
                                                 const QSet<Utils::FilePath> &filesInProject);
    ParserTreeItem::ConstPtr getCachedOrParseProjectTree(const Utils::FilePath &projectPath,
                                                         const QSet<Utils::FilePath> &filesInProject);
    ParserTreeItem::ConstPtr parse();

    //! Private class data pointer
    ParserPrivate *d;
};

} // namespace Internal
} // namespace ClassView
