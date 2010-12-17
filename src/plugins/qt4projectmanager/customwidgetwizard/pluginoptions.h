/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef PLUGINOPTIONS_H
#define PLUGINOPTIONS_H

#include <QtCore/QString>
#include <QtCore/QList>

namespace Qt4ProjectManager {
namespace Internal {

struct PluginOptions {
    QString pluginName;
    QString resourceFile;
    QString collectionClassName;
    QString collectionHeaderFile;
    QString collectionSourceFile;

    struct WidgetOptions {
        enum { LinkLibrary, IncludeProject } sourceType;
        QString widgetLibrary;
        QString widgetProjectFile;
        QString widgetClassName;
        QString widgetHeaderFile;
        QString widgetSourceFile;
        QString widgetBaseClassName;
        QString pluginClassName;
        QString pluginHeaderFile;
        QString pluginSourceFile;
        QString iconFile;
        bool createSkeleton;

        QString group;
        QString toolTip;
        QString whatsThis;
        bool isContainer;

        QString domXml;
    };

    QList<WidgetOptions> widgetOptions;
};

}
}

#endif
