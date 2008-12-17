/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef GCCPARSER_H
#define GCCPARSER_H

#include <projectexplorer/ProjectExplorerInterfaces>

#include <QtCore/QRegExp>

namespace Qt4ProjectManager {

class GccParser : public ProjectExplorer::BuildParserInterface
{
    Q_OBJECT

public:
    GccParser();
    QString name() const;
    virtual void stdOutput(const QString & line);
    virtual void stdError(const QString & line);
private:
    QRegExp m_regExp;
    QRegExp m_regExpIncluded;
    QRegExp m_regExpLinker;
    QRegExp m_makeDir;
    bool m_linkIndent;
};

} // namespace ProjectExplorer

#endif // GCCPARSER_H
