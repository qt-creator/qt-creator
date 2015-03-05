/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef DOXYGENGENERATOR_H
#define DOXYGENGENERATOR_H

#include "cpptools_global.h"

#include <cplusplus/Overview.h>

QT_FORWARD_DECLARE_CLASS(QTextCursor)

namespace CPlusPlus { class DeclarationAST; }

namespace CppTools {

class CPPTOOLS_EXPORT DoxygenGenerator
{
public:
    DoxygenGenerator();

    enum DocumentationStyle {
        JavaStyle,  ///< JavaStyle comment: /**
        QtStyle,    ///< QtStyle comment: /*!
        CppStyleA,  ///< CppStyle comment variant A: ///
        CppStyleB   ///< CppStyle comment variant B: //!
    };

    void setStyle(DocumentationStyle style);
    void setStartComment(bool start);
    void setGenerateBrief(bool gen);
    void setAddLeadingAsterisks(bool add);

    QString generate(QTextCursor cursor);
    QString generate(QTextCursor cursor, CPlusPlus::DeclarationAST *decl);

private:
    QChar startMark() const;
    QChar styleMark() const;

    enum Command {
        BriefCommand,
        ParamCommand,
        ReturnCommand
    };
    static QString commandSpelling(Command command);

    void writeStart(QString *comment) const;
    void writeEnd(QString *comment) const;
    void writeContinuation(QString *comment) const;
    void writeNewLine(QString *comment) const;
    void writeCommand(QString *comment,
                      Command command,
                      const QString &commandContent = QString()) const;
    void writeBrief(QString *comment,
                    const QString &brief,
                    const QString &prefix = QString(),
                    const QString &suffix = QString());

    void assignCommentOffset(QTextCursor cursor);
    QString offsetString() const;

    bool m_addLeadingAsterisks;
    bool m_generateBrief;
    bool m_startComment;
    DocumentationStyle m_style;
    CPlusPlus::Overview m_printer;
    QString m_commentOffset;
};

} // namespace CppTools

#endif // DOXYGENGENERATOR_H
