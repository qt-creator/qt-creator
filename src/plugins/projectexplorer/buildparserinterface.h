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
    IBuildParserFactory() {}
    virtual ~IBuildParserFactory();
    virtual bool canCreate(const QString & name) const = 0;
    virtual BuildParserInterface * create(const QString & name) const = 0;
};

} // namespace ProjectExplorer

#endif // BUILDPARSERINTERFACE_H
