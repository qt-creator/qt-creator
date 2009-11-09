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

#ifndef ABLDPARSER_H
#define ABLDPARSER_H

#include <projectexplorer/buildparserinterface.h>

#include <QtCore/QRegExp>

namespace Qt4ProjectManager {

class AbldParser : public ProjectExplorer::BuildParserInterface
{
    Q_OBJECT

public:
    AbldParser(const QString & name);
    ~AbldParser();

    QString name() const;
    virtual void stdOutput(const QString & line);
    virtual void stdError(const QString & line);
private:
    ProjectExplorer::BuildParserInterface * m_subparser;
    const QString m_name;
};

} // namespace Qt4ProjectExplorer

#endif // ABLDPARSER_H
