// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>
#include <utils/qtcassert.h>

#include <llvm/Support/VirtualFileSystem.h>

namespace ClangFormat::Internal {

using namespace llvm;
using namespace llvm::vfs;
using namespace llvm::sys::fs;
using namespace Utils;

class LlvmFileAdapter : public File
{
public:
    LlvmFileAdapter(const Twine &Path)
    {
        Q_UNUSED(Path);
    }
    /// Destroy the file after closing it (if open).
    /// Sub-classes should generally call close() inside their destructors. We
    /// cannot do that from the base class, since close is virtual.
    ~LlvmFileAdapter() { close(); }

    /// Get the status of the file.
    ErrorOr<Status> status() override
    {
        Q_UNIMPLEMENTED();
        return std::make_error_code(std::errc::not_supported);
    }

    /// Get the name of the file
    ErrorOr<std::string> getName() override
    {
        Q_UNIMPLEMENTED();
        return std::make_error_code(std::errc::not_supported);
    }

    /// Get the contents of the file as a \p MemoryBuffer.
    ErrorOr<std::unique_ptr<MemoryBuffer>> getBuffer(const Twine &Name,
                                                     int64_t FileSize = -1,
                                                     bool RequiresNullTerminator = true,
                                                     bool IsVolatile = false) override
    {
        Q_UNUSED(RequiresNullTerminator);
        Q_UNUSED(IsVolatile);

        const FilePath path = FilePath::fromUserInput(QString::fromStdString(Name.str()));
        const expected_str<QByteArray> contents = path.fileContents(FileSize, 0);
        QTC_ASSERT_EXPECTED(contents, return std::error_code());

        return MemoryBuffer::getMemBufferCopy(contents->data(), Name);
    }

    /// Closes the file.
    std::error_code close() override { return {}; }

protected:
};

class LlvmFileSystemAdapter : public FileSystem
{
public:
    LlvmFileSystemAdapter()
        : m_workingDirectory(FilePath::currentWorkingPath())
    {}

    ErrorOr<Status> status(const Twine &Path) override
    {
        const FilePath path = FilePath::fromUserInput(QString::fromStdString(Path.str()));

        QFileInfo fInfo(QString::fromStdString(Path.str()));
        if (!fInfo.exists())
            return std::error_code();

        const std::chrono::seconds modInSeconds = std::chrono::seconds(
            fInfo.lastModified().toSecsSinceEpoch());
        const std::chrono::time_point<std::chrono::system_clock> modTime(modInSeconds);
        file_type type = file_type::status_error;
        if (fInfo.isDir())
            type = file_type::directory_file;
        else if (fInfo.isFile())
            type = file_type::regular_file;
        else
            QTC_ASSERT(false, return std::error_code());

        Status result(Path, {0, 0}, modTime, 0, 0, fInfo.size(), type, perms::all_perms);
        return result;
    };

    virtual ErrorOr<std::unique_ptr<File>> openFileForRead(const Twine &Path) override
    {
        return std::make_unique<LlvmFileAdapter>(Path);
    }

    /// Get a directory_iterator for \p Dir.
    /// \note The 'end' iterator is directory_iterator().
    vfs::directory_iterator dir_begin(const Twine &Dir, std::error_code &EC) override
    {
        Q_UNUSED(Dir);
        Q_UNUSED(EC);
        Q_UNIMPLEMENTED();
        return {};
    }

    /// Set the working directory. This will affect all following operations on
    /// this file system and may propagate down for nested file systems.
    std::error_code setCurrentWorkingDirectory(const Twine &Path) override
    {
        Q_UNUSED(Path);
        Q_UNIMPLEMENTED();
        return std::make_error_code(std::errc::not_supported);
    }

    /// Get the working directory of this file system.
    ErrorOr<std::string> getCurrentWorkingDirectory() const override
    {
        Q_UNIMPLEMENTED();
        return std::make_error_code(std::errc::not_supported);
    }

    /// Gets real path of \p Path e.g. collapse all . and .. patterns, resolve
    /// symlinks. For real file system, this uses `llvm::sys::fs::real_path`.
    /// This returns errc::operation_not_permitted if not implemented by subclass.
    std::error_code getRealPath(const Twine &Path, SmallVectorImpl<char> &Output) const override
    {
        Q_UNUSED(Path);
        Q_UNUSED(Output);

        Q_UNIMPLEMENTED();
        return std::make_error_code(std::errc::not_supported);
    }

    /// Is the file mounted on a local filesystem?
    std::error_code isLocal(const Twine &Path, bool &Result) override
    {
        const FilePath filePath = FilePath::fromString(QString::fromStdString(Path.str()));
        Result = !filePath.needsDevice();
        return {};
    }

    std::error_code makeAbsolute(SmallVectorImpl<char> &Path) const override
    {
        if (Path.empty()) {
            const std::string asString = m_workingDirectory.toFSPathString().toStdString();
            Path.assign(asString.begin(), asString.end());
            return {};
        }
        const FilePath filePath = FilePath::fromString(QString::fromStdString(
                                                       std::string(Path.data(), Path.size())));
        if (filePath.isRelativePath()) {
            const std::string asString
                = m_workingDirectory.resolvePath(filePath).toFSPathString().toStdString();
            Path.assign(asString.begin(), asString.end());
        }
        return {};
    }

private:
    FilePath m_workingDirectory;
};

} // namespace ClangFormat::Internal
