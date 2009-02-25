/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "buildparserfactory.h"

#include "qt4projectmanagerconstants.h"
#include "gccparser.h"
#include "msvcparser.h"

using namespace Qt4ProjectManager::Internal;

GccParserFactory::~GccParserFactory()
{
}

bool GccParserFactory::canCreate(const QString & name) const
{
    return (name == Constants::BUILD_PARSER_GCC);
}

ProjectExplorer::BuildParserInterface * GccParserFactory::create(const QString & name) const
{
    Q_UNUSED(name);
    return new GccParser();
}

MsvcParserFactory::~MsvcParserFactory()
{
}

bool MsvcParserFactory::canCreate(const QString & name) const
{
    return (name == Constants::BUILD_PARSER_MSVC);
}

ProjectExplorer::BuildParserInterface * MsvcParserFactory::create(const QString & name) const
{
    Q_UNUSED(name);
    return new MsvcParser();
}
