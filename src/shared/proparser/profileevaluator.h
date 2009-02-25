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

#ifndef PROFILEEVALUATOR_H
#define PROFILEEVALUATOR_H

#include "proitems.h"
#include "abstractproitemvisitor.h"

#include <QtCore/QIODevice>
#include <QtCore/QHash>
#include <QtCore/QStringList>
#include <QtCore/QStack>

QT_BEGIN_NAMESPACE

class ProFile;
class ProFileEvaluator;

void evaluateProFile(const ProFileEvaluator &visitor, QHash<QByteArray, QStringList> *varMap);
bool evaluateProFile(const QString &fileName, bool verbose, QHash<QByteArray, QStringList> *varMap);

class ProFileEvaluator
{
public:
    enum TemplateType {
        TT_Unknown = 0,
        TT_Application,
        TT_Library,
        TT_Script,
        TT_Subdirs
    };

    ProFileEvaluator();
    virtual ~ProFileEvaluator();

    ProFileEvaluator::TemplateType templateType();
    virtual bool contains(const QString &variableName) const;
    void setVerbose(bool on); // Default is false
    void setCumulative(bool on); // Default is true!
    void setOutputDir(const QString &dir); // Default is empty

    bool queryProFile(ProFile *pro);
    bool accept(ProFile *pro);

    void addVariables(const QHash<QString, QStringList> &variables);
    void addProperties(const QHash<QString, QString> &properties);
    QStringList values(const QString &variableName) const;
    QStringList values(const QString &variableName, const ProFile *pro) const;
    QString propertyValue(const QString &val) const;

    // for our descendents
    virtual ProFile *parsedProFile(const QString &fileName);
    virtual void releaseParsedProFile(ProFile *proFile);
    virtual void logMessage(const QString &msg);
    virtual void errorMessage(const QString &msg); // .pro parse errors
    virtual void fileMessage(const QString &msg); // error() and message() from .pro file

private:
    class Private;
    Private *d;
};

QT_END_NAMESPACE

#endif // PROFILEEVALUATOR_H
