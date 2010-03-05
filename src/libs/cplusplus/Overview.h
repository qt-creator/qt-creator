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

#ifndef OVERVIEW_H
#define OVERVIEW_H

#include <CPlusPlusForwardDeclarations.h>
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

    bool showFullyQualifiedNames() const;
    void setShowFullyQualifiedNamed(bool showFullyQualifiedNames);

    // argument index that you want to mark
    unsigned markedArgument() const;
    void setMarkedArgument(unsigned position);

    int markedArgumentBegin() const;
    void setMarkedArgumentBegin(int begin);

    int markedArgumentEnd() const;
    void setMarkedArgumentEnd(int end);

    QString operator()(const Name *name) const
    { return prettyName(name); }

    QString operator()(const FullySpecifiedType &type, const Name *name = 0) const
    { return prettyType(type, name); }

    QString prettyName(const Name *name) const;
    QString prettyType(const FullySpecifiedType &type, const Name *name = 0) const;
    QString prettyType(const FullySpecifiedType &type, const QString &name) const;

private:
    unsigned _markedArgument;
    int _markedArgumentBegin;
    int _markedArgumentEnd;
    bool _showArgumentNames: 1;
    bool _showReturnTypes: 1;
    bool _showFunctionSignatures: 1;
    bool _showFullyQualifiedNames: 1;
};

} // end of namespace CPlusPlus

#endif // OVERVIEW_H
