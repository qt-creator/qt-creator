// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectserializer.h"

#include "infrastructureserializer.h"

#include "qmt/project/project.h"
#include "qmt/model/mpackage.h"

#include "qark/qxmloutarchive.h"
#include "qark/qxmlinarchive.h"
#include "qark/serialize.h"

#include "qmt/infrastructure/ioexceptions.h"

#ifdef USE_COMPRESSED_FILES
#include "infrastructure/qcompressedfile.h"
#endif

#include <QFile>

namespace qark {

using namespace qmt;

QARK_REGISTER_TYPE_NAME(Project, "Project")

template<class Archive>
void serialize(Archive &archive, Project &project)
{
    archive || qark::tag("project", project)
            || qark::attr("uid", project, &Project::uid, &Project::setUid)
            || qark::attr("root-package", project, &Project::rootPackage, &Project::setRootPackage)
            || qark::attr("config-path", project, &Project::configPath, &Project::setConfigPath)
            || qark::end;
}

} // namespace qark

namespace qmt {

ProjectSerializer::ProjectSerializer()
{
}

ProjectSerializer::~ProjectSerializer()
{
}

void ProjectSerializer::save(const QString &fileName, const Project *project)
{
    QMT_ASSERT(project, return);

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly))
        throw FileCreationException(fileName);

    QIODevice *xmlDevice = &file;

#ifdef USE_COMPRESSED_FILES
    qmt::QCompressedDevice compressor(&file);
    if (!compressor.open(QIODevice::WriteOnly))
        throw IOException("Unable to create compressed file");
    xmlDevice = &compressor;
#endif

    QXmlStreamWriter writer(xmlDevice);
    write(&writer, project);

#ifdef USE_COMPRESSED_FILES
    compressor.close();
#endif
    file.close();
}

QByteArray ProjectSerializer::save(const Project *project)
{
    QByteArray buffer;

    QXmlStreamWriter writer(&buffer);
    write(&writer, project);

    return buffer;
}

void ProjectSerializer::load(const QString &fileName, Project *project)
{
    QMT_ASSERT(project, return);

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
        throw FileNotFoundException(fileName);

    QIODevice *xmlDevice = &file;

#ifdef USE_COMPRESSED_FILES
    qmt::QCompressedDevice uncompressor(&file);
    if (!uncompressor.open(QIODevice::ReadOnly))
        throw IOException("Unable to access compressed file");
    xmlDevice = &uncompressor;
#endif

    QXmlStreamReader reader(xmlDevice);

    try {
        qark::QXmlInArchive archive(reader);
        archive.beginDocument();
        archive >> qark::tag("qmt");
        archive >> *project;
        archive >> qark::end;
        archive.endDocument();
    } catch (const qark::QXmlInArchive::FileFormatException &) {
        throw FileIOException("illegal file format", fileName);
    } catch (...) {
        throw FileIOException("serialization error", fileName);
    }

#ifdef USE_COMPRESSED_FILES
    uncompressor.close();
#endif
    file.close();
}

void ProjectSerializer::write(QXmlStreamWriter *writer, const Project *project)
{
    writer->setAutoFormatting(true);
    writer->setAutoFormattingIndent(1);

    try {
        qark::QXmlOutArchive archive(*writer);
        archive.beginDocument();
        archive << qark::tag("qmt");
        archive << *project;
        archive << qark::end;
        archive.endDocument();
    } catch (...) {
        throw IOException("serialization error");
    }
}

} // namespace qmt
