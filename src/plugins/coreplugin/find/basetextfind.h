// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "ifindsupport.h"

#include <utils/multitextcursor.h>

#include <QRegularExpression>

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
class QTextEdit;
class QTextCursor;
QT_END_NAMESPACE

namespace Core {
struct BaseTextFindPrivate;

class CORE_EXPORT BaseTextFind : public IFindSupport
{
    Q_OBJECT

public:
    explicit BaseTextFind(QPlainTextEdit *editor);
    explicit BaseTextFind(QTextEdit *editor);
    ~BaseTextFind() override;

    bool supportsReplace() const override;
    Utils::FindFlags supportedFindFlags() const override;
    void resetIncrementalSearch() override;
    void clearHighlights() override;
    QString currentFindString() const override;
    QString completedFindString() const override;

    Result findIncremental(const QString &txt, Utils::FindFlags findFlags) override;
    Result findStep(const QString &txt, Utils::FindFlags findFlags) override;
    void replace(const QString &before, const QString &after, Utils::FindFlags findFlags) override;
    bool replaceStep(const QString &before, const QString &after,
                     Utils::FindFlags findFlags) override;
    int replaceAll(const QString &before, const QString &after,
                   Utils::FindFlags findFlags) override;

    void defineFindScope() override;
    void clearFindScope() override;

    void highlightAll(const QString &txt, Utils::FindFlags findFlags) override;

    using CursorProvider = std::function<Utils::MultiTextCursor ()>;
    void setMultiTextCursorProvider(const CursorProvider &provider);
    bool inScope(const QTextCursor &candidate) const;
    bool inScope(int candidateStart, int candidateEnd) const;

    static QRegularExpression regularExpression(const QString &txt, Utils::FindFlags flags);

signals:
    void highlightAllRequested(const QString &txt, Utils::FindFlags findFlags);
    void findScopeChanged(const Utils::MultiTextCursor &cursor);

private:
    bool find(const QString &txt, Utils::FindFlags findFlags, QTextCursor start, bool *wrapped);
    QTextCursor replaceInternal(const QString &before, const QString &after,
                                Utils::FindFlags findFlags);

    Utils::MultiTextCursor multiTextCursor() const;
    QTextCursor textCursor() const;
    void setTextCursor(const QTextCursor&);
    QTextDocument *document() const;
    bool isReadOnly() const;
    QTextCursor findOne(const QRegularExpression &expr, QTextCursor from,
                        QTextDocument::FindFlags options) const;

    BaseTextFindPrivate *d;
};

} // namespace Core
