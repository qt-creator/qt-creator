// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppprojectfile.h"

#include "cppeditorconstants.h"

#include <coreplugin/icore.h>
#include <utils/filepath.h>
#include <utils/mimeutils.h>

#include <QDebug>

namespace CppEditor {

ProjectFile::ProjectFile(const Utils::FilePath &filePath, Kind kind, bool active)
    : path(filePath)
    , kind(kind)
    , active(active)
{
}

bool ProjectFile::operator==(const ProjectFile &other) const
{
    return active == other.active
        && kind == other.kind
        && path == other.path;
}

ProjectFile::Kind ProjectFile::classifyByMimeType(const QString &mt)
{
    if (mt == CppEditor::Constants::C_SOURCE_MIMETYPE)
        return CSource;
    if (mt == CppEditor::Constants::C_HEADER_MIMETYPE)
        return CHeader;
    if (mt == CppEditor::Constants::CPP_SOURCE_MIMETYPE)
        return CXXSource;
    if (mt == CppEditor::Constants::CPP_HEADER_MIMETYPE)
        return CXXHeader;
    if (mt == CppEditor::Constants::OBJECTIVE_C_SOURCE_MIMETYPE)
        return ObjCSource;
    if (mt == CppEditor::Constants::OBJECTIVE_CPP_SOURCE_MIMETYPE)
        return ObjCXXSource;
    if (mt == CppEditor::Constants::QDOC_MIMETYPE)
        return CXXSource;
    if (mt == CppEditor::Constants::MOC_MIMETYPE)
        return CXXSource;
    if (mt == CppEditor::Constants::CUDA_SOURCE_MIMETYPE)
        return CudaSource;
    if (mt == CppEditor::Constants::AMBIGUOUS_HEADER_MIMETYPE)
        return AmbiguousHeader;
    return Unsupported;
}

ProjectFile::Kind ProjectFile::classify(const QString &filePath)
{
    if (isAmbiguousHeader(filePath))
        return AmbiguousHeader;

    const Utils::MimeType mimeType = Utils::mimeTypeForFile(filePath);
    return classifyByMimeType(mimeType.name());
}

bool ProjectFile::isAmbiguousHeader(const QString &filePath)
{
    return filePath.endsWith(".h");
}

bool ProjectFile::isObjC(const QString &filePath)
{
    return isObjC(classify(filePath));
}

bool ProjectFile::isObjC(Kind kind)
{
    switch (kind) {
    case CppEditor::ProjectFile::ObjCHeader:
    case CppEditor::ProjectFile::ObjCXXHeader:
    case CppEditor::ProjectFile::ObjCSource:
    case CppEditor::ProjectFile::ObjCXXSource:
        return true;
    default:
        return false;
    }
}

ProjectFile::Kind ProjectFile::sourceForHeaderKind(ProjectFile::Kind kind)
{
    ProjectFile::Kind sourceKind;
    switch (kind) {
    case ProjectFile::CHeader:
        sourceKind = ProjectFile::CSource;
        break;
    case ProjectFile::ObjCHeader:
        sourceKind = ProjectFile::ObjCSource;
        break;
    case ProjectFile::ObjCXXHeader:
        sourceKind = ProjectFile::ObjCXXSource;
        break;
    case ProjectFile::Unsupported: // no file extension, e.g. stl headers
    case ProjectFile::AmbiguousHeader:
    case ProjectFile::CXXHeader:
    default:
        sourceKind = ProjectFile::CXXSource;
    }

    return sourceKind;
}

ProjectFile::Kind ProjectFile::sourceKind(Kind kind)
{
    ProjectFile::Kind sourceKind = kind;
    if (ProjectFile::isHeader(kind))
        sourceKind = ProjectFile::sourceForHeaderKind(kind);
    return sourceKind;
}

bool ProjectFile::isHeader(ProjectFile::Kind kind)
{
    switch (kind) {
    case ProjectFile::CHeader:
    case ProjectFile::CXXHeader:
    case ProjectFile::ObjCHeader:
    case ProjectFile::ObjCXXHeader:
    case ProjectFile::Unsupported: // no file extension, e.g. stl headers
    case ProjectFile::AmbiguousHeader:
        return true;
    default:
        return false;
    }
}

bool ProjectFile::isHeader(const Utils::FilePath &fp)
{
    return isHeader(classify(fp.toString()));
}

bool ProjectFile::isSource(ProjectFile::Kind kind)
{
    switch (kind) {
    case ProjectFile::CSource:
    case ProjectFile::CXXSource:
    case ProjectFile::ObjCSource:
    case ProjectFile::ObjCXXSource:
    case ProjectFile::CudaSource:
    case ProjectFile::OpenCLSource:
        return true;
    default:
        return false;
    }
}

bool ProjectFile::isHeader() const
{
    return isHeader(kind);
}

bool ProjectFile::isSource() const
{
    return isSource(kind);
}

bool ProjectFile::isC(ProjectFile::Kind kind)
{
    switch (kind) {
    case ProjectFile::CHeader:
    case ProjectFile::CSource:
    case ProjectFile::ObjCHeader:
    case ProjectFile::ObjCSource:
        return true;
    default:
        return false;
    }
}

bool ProjectFile::isCxx(ProjectFile::Kind kind)
{
    switch (kind) {
    case ProjectFile::CXXHeader:
    case ProjectFile::CXXSource:
    case ProjectFile::ObjCXXHeader:
    case ProjectFile::ObjCXXSource:
    case ProjectFile::CudaSource:
        return true;
    default:
        return false;
    }
}

bool ProjectFile::isC() const
{
    return isC(kind);
}

bool ProjectFile::isCxx() const
{
    return isCxx(kind);
}

#define RETURN_TEXT_FOR_CASE(enumValue) case ProjectFile::enumValue: return #enumValue
const char *projectFileKindToText(ProjectFile::Kind kind)
{
    switch (kind) {
        RETURN_TEXT_FOR_CASE(Unclassified);
        RETURN_TEXT_FOR_CASE(Unsupported);
        RETURN_TEXT_FOR_CASE(AmbiguousHeader);
        RETURN_TEXT_FOR_CASE(CHeader);
        RETURN_TEXT_FOR_CASE(CSource);
        RETURN_TEXT_FOR_CASE(CXXHeader);
        RETURN_TEXT_FOR_CASE(CXXSource);
        RETURN_TEXT_FOR_CASE(ObjCHeader);
        RETURN_TEXT_FOR_CASE(ObjCSource);
        RETURN_TEXT_FOR_CASE(ObjCXXHeader);
        RETURN_TEXT_FOR_CASE(ObjCXXSource);
        RETURN_TEXT_FOR_CASE(CudaSource);
        RETURN_TEXT_FOR_CASE(OpenCLSource);
    }

    return "UnhandledProjectFileKind";
}
#undef RETURN_TEXT_FOR_CASE

QDebug operator<<(QDebug stream, const ProjectFile &projectFile)
{
    stream << projectFile.path << QLatin1String(", ") << projectFileKindToText(projectFile.kind);
    return stream;
}

} // namespace CppEditor
