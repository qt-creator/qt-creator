// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <vcsbase/vcsbaseeditor.h>

#include <QRegularExpression>

namespace Utils {
class FancyLineEdit;
class FilePath;
} // Utils

namespace Git::Internal {

class GitLogFilterWidget;

class GitEditorWidget : public VcsBase::VcsBaseEditorWidget
{
    Q_OBJECT

public:
    GitEditorWidget();

    void setPlainText(const QString &text) override;
    QWidget *addFilterWidget();
    void setPickaxeLineEdit(Utils::FancyLineEdit *lineEdit);
    QString grepValue() const;
    QString pickaxeValue() const;
    QString authorValue() const;
    bool caseSensitive() const;
    void refresh();

signals:
    void toggleFilters(bool value);

private:
    void applyDiffChunk(const VcsBase::DiffChunk& chunk, Core::PatchAction patchAction);

    void init() override;
    void addDiffActions(QMenu *menu, const VcsBase::DiffChunk &chunk) override;
    void aboutToOpen(const Utils::FilePath &filePath, const Utils::FilePath &realFilePath) override;
    QString changeUnderCursor(const QTextCursor &) const override;
    VcsBase::BaseAnnotationHighlighter *createAnnotationHighlighter(const QSet<QString> &changes) const override;
    QString decorateVersion(const QString &revision) const override;
    QStringList annotationPreviousVersions(const QString &revision) const override;
    bool isValidRevision(const QString &revision) const override;
    void addChangeActions(QMenu *menu, const QString &change) override;
    QString revisionSubject(const QTextBlock &inBlock) const override;
    bool supportChangeLinks() const override;
    Utils::FilePath fileNameForLine(int line) const override;
    Utils::FilePath sourceWorkingDirectory() const;

    const QRegularExpression m_changeNumberPattern;
    GitLogFilterWidget *m_logFilterWidget = nullptr;
};

} // Git::Internal
