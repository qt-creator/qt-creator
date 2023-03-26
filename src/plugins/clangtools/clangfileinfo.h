// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cppeditor/cppprojectfile.h>
#include <cppeditor/projectpart.h>

#include <utils/fileutils.h>

#include <QSet>

namespace ClangTools {
namespace Internal {

class FileInfo
{
public:
    FileInfo() = default;
    FileInfo(Utils::FilePath file,
             CppEditor::ProjectFile::Kind kind,
             CppEditor::ProjectPart::ConstPtr projectPart)
        : file(std::move(file))
        , kind(kind)
        , projectPart(projectPart)
    {}

    friend bool operator==(const FileInfo &lhs, const FileInfo &rhs) {
        return lhs.file == rhs.file;
    }

    Utils::FilePath file;
    CppEditor::ProjectFile::Kind kind;
    CppEditor::ProjectPart::ConstPtr projectPart;
};
using FileInfos = std::vector<FileInfo>;

class FileInfoSelection {
public:
    QSet<Utils::FilePath> dirs;
    QSet<Utils::FilePath> files;
};

class FileInfoProvider {
public:
    QString displayName;
    FileInfos fileInfos;
    FileInfoSelection selection;

    enum ExpandPolicy {
        All,
        Limited,
    } expandPolicy = All;

    using OnSelectionAccepted = std::function<void(const FileInfoSelection &selection)>;
    OnSelectionAccepted onSelectionAccepted;
};
using FileInfoProviders = std::vector<FileInfoProvider>;

} // namespace Internal
} // namespace ClangTools
