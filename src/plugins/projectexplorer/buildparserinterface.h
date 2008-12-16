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

#ifndef BUILDPARSERINTERFACE_H
#define BUILDPARSERINTERFACE_H

#include "projectexplorer_export.h"

#include <QtCore/QObject>
#include <QtCore/QStack>

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT BuildParserInterface : public QObject
{
    Q_OBJECT
public:
    enum PatternType { Unknown, Warning, Error };

    virtual ~BuildParserInterface() {}
    virtual QString name() const = 0;

    virtual void stdOutput(const QString & line) = 0;
    virtual void stdError(const QString & line) = 0;

Q_SIGNALS:
    void enterDirectory(const QString &dir);
    void leaveDirectory(const QString &dir);
    void addToOutputWindow(const QString & string);
    void addToTaskWindow(const QString & filename, int type, int lineNumber, const QString & description);
};

class PROJECTEXPLORER_EXPORT IBuildParserFactory
    : public QObject
{
    Q_OBJECT

public:
    IBuildParserFactory() {};
    virtual ~IBuildParserFactory();
    virtual bool canCreate(const QString & name) const = 0;
    virtual BuildParserInterface * create(const QString & name) const = 0;
};

} // namespace ProjectExplorer

#endif // BUILDPARSERINTERFACE_H
