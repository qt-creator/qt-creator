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
    archive || qark::tag(QStringLiteral("project"), project)
            || qark::attr(QStringLiteral("uid"), project, &Project::uid, &Project::setUid)
            || qark::attr(QStringLiteral("root-package"), project, &Project::rootPackage, &Project::setRootPackage)
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
    QMT_CHECK(project);

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
    QMT_CHECK(project);

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
        throw FileIOException(QStringLiteral("illegal file format"), fileName);
    } catch (...) {
        throw FileIOException(QStringLiteral("serialization error"), fileName);
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
        throw IOException(QStringLiteral("serialization error"));
    }
}

} // namespace qmt
