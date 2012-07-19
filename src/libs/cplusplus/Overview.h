/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef CPLUSPLUS_OVERVIEW_H
#define CPLUSPLUS_OVERVIEW_H

#include <CPlusPlusForwardDeclarations.h>

#include <QList>
#include <QString>

namespace CPlusPlus {

class CPLUSPLUS_EXPORT Overview
{
    Overview(const Overview &other);
    void operator =(const Overview &other);

public:
    Overview();
    ~Overview();

    bool showArgumentNames() const;
    void setShowArgumentNames(bool showArgumentNames);

    bool showReturnTypes() const;
    void setShowReturnTypes(bool showReturnTypes);

    bool showFunctionSignatures() const;
    void setShowFunctionSignatures(bool showFunctionSignatures);

    bool showDefaultArguments() const;
    void setShowDefaultArguments(bool showDefaultArguments);

    bool showTemplateParameters() const;
    void setShowTemplateParameters(bool showTemplateParameters);

    // argument index that you want to mark
    unsigned markedArgument() const;
    void setMarkedArgument(unsigned position);

    int markedArgumentBegin() const;
    void setMarkedArgumentBegin(int begin);

    int markedArgumentEnd() const;
    void setMarkedArgumentEnd(int end);

    QString operator()(const Name *name) const
    { return prettyName(name); }

    QString operator()(const QList<const Name *> &fullyQualifiedName) const
    { return prettyName(fullyQualifiedName); }

    QString operator()(const FullySpecifiedType &type, const Name *name = 0) const
    { return prettyType(type, name); }

    QString prettyName(const Name *name) const;
    QString prettyName(const QList<const Name *> &fullyQualifiedName) const;
    QString prettyType(const FullySpecifiedType &type, const Name *name = 0) const;
    QString prettyType(const FullySpecifiedType &type, const QString &name) const;

private:
    unsigned _markedArgument;
    int _markedArgumentBegin;
    int _markedArgumentEnd;
    bool _showArgumentNames: 1;
    bool _showReturnTypes: 1;
    bool _showFunctionSignatures: 1;
    bool _showDefaultArguments: 1;
    bool _showTemplateParameters: 1;
};

} // namespace CPlusPlus

#endif // CPLUSPLUS_OVERVIEW_H
