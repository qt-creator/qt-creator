/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "s60buildparserfactory.h"
#include "abldparser.h"
#include "rvctparser.h"

#include <projectexplorer/projectexplorerconstants.h>

using namespace Qt4ProjectManager::Internal;

S60ParserFactory::~S60ParserFactory()
{
}

bool S60ParserFactory::canCreate(const QString & name) const
{
    return ((name == QLatin1String(ProjectExplorer::Constants::BUILD_PARSER_ABLD_GCCE)) ||
            (name == QLatin1String(ProjectExplorer::Constants::BUILD_PARSER_ABLD_WINSCW)) ||
            (name == QLatin1String(ProjectExplorer::Constants::BUILD_PARSER_ABLD_RVCT))  ||
            (name == QLatin1String(ProjectExplorer::Constants::BUILD_PARSER_RVCT)));
}

ProjectExplorer::BuildParserInterface * S60ParserFactory::create(const QString & name) const
{
    if (name ==  QLatin1String(ProjectExplorer::Constants::BUILD_PARSER_RVCT))
    {
        return new RvctParser();
    }

    return new AbldParser(name);
}

