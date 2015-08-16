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

#include "diagramreferenceserializer.h"

#include "infrastructureserializer.h"

#include "qmt/project/project.h"
#include "qmt/model/mdiagram.h"

#include "qark/qxmloutarchive.h"
#include "qark/qxmlinarchive.h"
#include "qark/serialize.h"

#include "qmt/infrastructure/ioexceptions.h"

#include <QFile>


namespace qark {

using namespace qmt;

QARK_REGISTER_TYPE_NAME(DiagramReferenceSerializer::Reference, "DiagramReferenceSerializer--Reference")

template<class Archive>
void serialize(Archive &archive, DiagramReferenceSerializer::Reference &reference)
{
    archive || qark::tag(QStringLiteral("diagram-reference"), reference)
            || qark::attr(QStringLiteral("model"), reference._model_uid)
            || qark::attr(QStringLiteral("diagram"), reference._diagram_uid)
            || qark::end;
}

}


namespace qmt {

DiagramReferenceSerializer::Reference::Reference()
{
}

DiagramReferenceSerializer::Reference::Reference(const Uid &model_uid, const Uid &diagram_uid)
    : _model_uid(model_uid),
      _diagram_uid(diagram_uid)
{
}



DiagramReferenceSerializer::DiagramReferenceSerializer()
{
}

DiagramReferenceSerializer::~DiagramReferenceSerializer()
{
}

void DiagramReferenceSerializer::save(const QString &file_name, const DiagramReferenceSerializer::Reference &reference)
{
    QFile file(file_name);
    if (!file.open(QIODevice::WriteOnly)) {
        throw FileCreationException(file_name);
    }
    QIODevice *xml_device = &file;
    QXmlStreamWriter writer(xml_device);
    write(reference, &writer);
}

QByteArray DiagramReferenceSerializer::save(const Project *project, const MDiagram *diagram)
{
    QByteArray buffer;

    QXmlStreamWriter writer(&buffer);
    write(Reference(project->getUid(), diagram->getUid()), &writer);

    return buffer;
}

DiagramReferenceSerializer::Reference DiagramReferenceSerializer::load(const QString &file_name)
{
    QFile file(file_name);
    if (!file.open(QIODevice::ReadOnly)) {
        throw FileNotFoundException(file_name);
    }
    QIODevice *xml_device = &file;
    QXmlStreamReader reader(xml_device);
    return read(&reader);
}

DiagramReferenceSerializer::Reference DiagramReferenceSerializer::load(const QByteArray &contents)
{
    QXmlStreamReader reader(contents);
    return read(&reader);
}

void DiagramReferenceSerializer::write(const Reference &reference, QXmlStreamWriter *writer)
{
    writer->setAutoFormatting(true);
    writer->setAutoFormattingIndent(2);

    qark::QXmlOutArchive archive(*writer);
    archive.beginDocument();
    archive << qark::tag("qmt-diagram-reference");
    archive << reference;
    archive << qark::end();
    archive.endDocument();
}

DiagramReferenceSerializer::Reference DiagramReferenceSerializer::read(QXmlStreamReader *stream_reader)
{
    Reference reference;

    qark::QXmlInArchive archive(*stream_reader);
    archive.beginDocument();
    archive >> qark::tag("qmt-diagram-reference");
    archive >> reference;
    archive >> qark::end;
    archive.endDocument();

    return reference;
}

}
