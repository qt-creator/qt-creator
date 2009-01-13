/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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

    void setMarkArgument(unsigned position); // 1-based

    QString operator()(Name *name) const
    { return prettyName(name); }

    QString operator()(const FullySpecifiedType &type, Name *name = 0) const
    { return prettyType(type, name); }

    QString prettyName(Name *name) const;
    QString prettyType(const FullySpecifiedType &type, Name *name = 0) const;
    QString prettyType(const FullySpecifiedType &type, const QString &name) const;

private:
    unsigned _markArgument;
    bool _showArgumentNames: 1;
    bool _showReturnTypes: 1;
    bool _showFunctionSignatures: 1;
};

} // end of namespace CPlusPlus

#endif // OVERVIEW_H
