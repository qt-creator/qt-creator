// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cplusplus/Overview.h>

QT_FORWARD_DECLARE_CLASS(QTextCursor)

namespace CPlusPlus { class DeclarationAST; }
namespace CPlusPlus { class Snapshot; }
namespace Utils { class FilePath; }

namespace CppEditor::Internal {

class DoxygenGenerator
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

    QString generate(QTextCursor cursor,
                     const CPlusPlus::Snapshot &snapshot,
                     const Utils::FilePath &documentFilePath);
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

    bool m_addLeadingAsterisks = true;
    bool m_generateBrief = true;
    bool m_startComment = true;
    DocumentationStyle m_style = QtStyle;
    CPlusPlus::Overview m_printer;
    QString m_commentOffset;
};

} // namespace CppEditor::Internal
