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

#ifndef QMLOUTPUTFORMATTER_H
#define QMLOUTPUTFORMATTER_H

#include <projectexplorer/outputformatter.h>
#include <QtCore/QRegExp>
#include <QtCore/QSharedPointer>
#include <QtGui/QTextCharFormat>

namespace Qt4ProjectManager
{
class Qt4Project;

struct LinkResult
{
    int start;
    int end;
    QString href;
};

class QtOutputFormatter: public ProjectExplorer::OutputFormatter
{
public:
    QtOutputFormatter(Qt4Project *project);

    virtual void appendApplicationOutput(const QString &text, bool onStdErr);

    virtual void handleLink(const QString &href);

private:
    LinkResult matchLine(const QString &line) const;
    void appendLine(LinkResult lr, const QString &line, bool onStdError);

    QRegExp m_qmlError;
    QRegExp m_qtError;
    QWeakPointer<Qt4Project> m_project;
    QTextCharFormat m_linkFormat;
    QString m_lastLine;
};


} // namespace QmlProjectManager

#endif // QMLOUTPUTFORMATTER_H
