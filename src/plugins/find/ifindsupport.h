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

#ifndef IFINDSUPPORT_H
#define IFINDSUPPORT_H

#include "find_global.h"
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtGui/QTextDocument>

namespace Find {

class FIND_EXPORT IFindSupport : public QObject
{
    Q_OBJECT

public:
    enum FindFlag {
        FindBackward = 0x01,
        FindCaseSensitively = 0x02,
        FindWholeWords = 0x04,
        FindRegularExpression = 0x08
    };
    Q_DECLARE_FLAGS(FindFlags, FindFlag)

    enum Result { Found, NotFound, NotYetFound };

    IFindSupport() : QObject(0) {}
    virtual ~IFindSupport() {}

    virtual bool supportsReplace() const = 0;
    virtual FindFlags supportedFindFlags() const = 0;
    virtual void resetIncrementalSearch() = 0;
    virtual void clearResults() = 0;
    virtual QString currentFindString() const = 0;
    virtual QString completedFindString() const = 0;

    virtual void highlightAll(const QString &txt, FindFlags findFlags);
    virtual Result findIncremental(const QString &txt, FindFlags findFlags) = 0;
    virtual Result findStep(const QString &txt, FindFlags findFlags) = 0;
    virtual bool replaceStep(const QString &before, const QString &after,
        FindFlags findFlags) = 0;
    virtual int replaceAll(const QString &before, const QString &after,
        FindFlags findFlags) = 0;

    virtual void defineFindScope(){}
    virtual void clearFindScope(){}

    static QTextDocument::FindFlags textDocumentFlagsForFindFlags(IFindSupport::FindFlags flags)
    {
        QTextDocument::FindFlags textDocFlags;
        if (flags&IFindSupport::FindBackward)
            textDocFlags |= QTextDocument::FindBackward;
        if (flags&IFindSupport::FindCaseSensitively)
            textDocFlags |= QTextDocument::FindCaseSensitively;
        if (flags&IFindSupport::FindWholeWords)
            textDocFlags |= QTextDocument::FindWholeWords;
        return textDocFlags;
    }

signals:
    void changed();
};

inline void IFindSupport::highlightAll(const QString &, FindFlags) {}

} // namespace Find

Q_DECLARE_OPERATORS_FOR_FLAGS(Find::IFindSupport::FindFlags)

#endif // IFINDSUPPORT_H
