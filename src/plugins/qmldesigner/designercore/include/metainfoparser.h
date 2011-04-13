/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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

class ItemLibraryEntry;

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
    void handleNodeElement(QXmlStreamReader &reader);
    void handleNodeItemLibraryEntryElement(QXmlStreamReader &reader, const QString &className, const QIcon &icon);
    void handleItemLibraryEntryPropertyElement(QXmlStreamReader &reader, ItemLibraryEntry &itemLibraryEntry);

private:
    MetaInfo m_metaInfo;
};

}
}
#endif // METAINFOPARSER_H
