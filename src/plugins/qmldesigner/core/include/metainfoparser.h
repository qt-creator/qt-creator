/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef METAINFOPARSER_H
#define METAINFOPARSER_H

#include "corelib_global.h"
#include <QtCore/QXmlStreamReader>
#include <QtCore/QString>
#include <QtCore/QFile>
#include <metainfo.h>

namespace QmlDesigner {

class NodeMetaInfo;
class EnumeratorMetaInfo;
class ItemLibraryInfo;

namespace Internal {


class TEST_CORESHARED_EXPORT MetaInfoParser
{
public:
    MetaInfoParser(const MetaInfo &metaInfo);

    void parseFile(const QString &path);

protected:
    void errorHandling(QXmlStreamReader &reader, QFile &file);
    void tokenHandler(QXmlStreamReader &reader);
    void metaInfoHandler(QXmlStreamReader &reader);
    void handleMetaInfoElement(QXmlStreamReader &reader);
    void handleEnumElement(QXmlStreamReader &reader);
    void handleEnumElementElement(QXmlStreamReader &reader, EnumeratorMetaInfo &enumeratorMetaInfo);
    void handleFlagElement(QXmlStreamReader &reader);
    void handleFlagElementElement(QXmlStreamReader &reader, EnumeratorMetaInfo &enumeratorMetaInfo);
    void handleNodeElement(QXmlStreamReader &reader);
    void handleNodeInheritElement(QXmlStreamReader &reader, const QString &className);
    void handleNodeItemLibraryRepresentationElement(QXmlStreamReader &reader, const QString &className);
    void handleAbstractPropertyElement(QXmlStreamReader &reader, NodeMetaInfo &nodeMetaInfo);
    void handleAbstractPropertyDefaultValueElement(QXmlStreamReader &reader, NodeMetaInfo &nodeMetaInfo);
    void handleItemLibraryInfoPropertyElement(QXmlStreamReader &reader, ItemLibraryInfo &ItemLibraryInfo);

private:
    MetaInfo m_metaInfo;
};

}
}
#endif // METAINFOPARSER_H
