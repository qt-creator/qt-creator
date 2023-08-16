// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "diffenums.h"

#include <utils/id.h>

#include <QIcon>
#include <QString>
#include <QObject>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

namespace TextEditor { class TextEditorWidget; }

namespace DiffEditor {

class FileData;

namespace Internal {

class DiffEditorDocument;
class SideBySideDiffEditorWidget;
class UnifiedDiffEditorWidget;

class IDiffView : public QObject
{
    Q_OBJECT

public:
    explicit IDiffView(QObject *parent = nullptr);

    QIcon icon() const;
    QString toolTip() const;
    bool supportsSync() const;
    QString syncToolTip() const;

    Utils::Id id() const;
    virtual QWidget *widget() = 0;
    virtual void setDocument(DiffEditorDocument *document) = 0;

    virtual void beginOperation() = 0;
    virtual void setCurrentDiffFileIndex(int index) = 0;
    virtual void setDiff(const QList<FileData> &diffFileList) = 0;
    virtual void setMessage(const QString &message) = 0;
    virtual void endOperation() = 0;

    virtual void setSync(bool) = 0;

signals:
    void currentDiffFileIndexChanged(int index);

protected:
    void setIcon(const QIcon &icon);
    void setToolTip(const QString &toolTip);
    void setId(const Utils::Id &id);
    void setSupportsSync(bool sync);
    void setSyncToolTip(const QString &text);

private:
    QIcon m_icon;
    QString m_toolTip;
    Utils::Id m_id;
    bool m_supportsSync = false;
    QString m_syncToolTip;
};

class UnifiedView : public IDiffView
{
    Q_OBJECT

public:
    UnifiedView();

    QWidget *widget() override;
    TextEditor::TextEditorWidget *textEditorWidget();

    void setDocument(DiffEditorDocument *document) override;

    void beginOperation() override;
    void setCurrentDiffFileIndex(int index) override;
    void setDiff(const QList<FileData> &diffFileList) override;
    void setMessage(const QString &message) override;
    void endOperation() override;

    void setSync(bool sync) override;

private:
    UnifiedDiffEditorWidget *m_widget = nullptr;
};

class SideBySideView : public IDiffView
{
    Q_OBJECT

public:
    SideBySideView();

    QWidget *widget() override;
    TextEditor::TextEditorWidget *sideEditorWidget(DiffSide side);

    void setDocument(DiffEditorDocument *document) override;

    void beginOperation() override;
    void setCurrentDiffFileIndex(int index) override;
    void setDiff(const QList<FileData> &diffFileList) override;
    void setMessage(const QString &message) override;
    void endOperation() override;

    void setSync(bool sync) override;

private:
    SideBySideDiffEditorWidget *m_widget;
};

} // namespace Internal
} // namespace DiffEditor
