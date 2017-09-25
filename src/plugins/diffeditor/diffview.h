/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <coreplugin/id.h>

#include <QIcon>
#include <QString>
#include <QObject>

QT_FORWARD_DECLARE_CLASS(QWidget)

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
    explicit IDiffView(QObject *parent = 0);

    QIcon icon() const;
    QString toolTip() const;
    bool supportsSync() const;
    QString syncToolTip() const;

    Core::Id id() const;
    virtual QWidget *widget() = 0;
    virtual void setDocument(DiffEditorDocument *document) = 0;

    virtual void beginOperation() = 0;
    virtual void setCurrentDiffFileIndex(int index) = 0;
    virtual void setDiff(const QList<FileData> &diffFileList, const QString &workingDirectory) = 0;
    virtual void endOperation(bool success) = 0;

    virtual void setSync(bool) = 0;

signals:
    void currentDiffFileIndexChanged(int index);

protected:
    void setIcon(const QIcon &icon);
    void setToolTip(const QString &toolTip);
    void setId(const Core::Id &id);
    void setSupportsSync(bool sync);
    void setSyncToolTip(const QString &text);

private:
    QIcon m_icon;
    QString m_toolTip;
    Core::Id m_id;
    bool m_supportsSync;
    QString m_syncToolTip;
};

class UnifiedView : public IDiffView
{
    Q_OBJECT

public:
    UnifiedView();

    QWidget *widget();
    TextEditor::TextEditorWidget *textEditorWidget();

    void setDocument(DiffEditorDocument *document);

    void beginOperation();
    void setCurrentDiffFileIndex(int index);
    void setDiff(const QList<FileData> &diffFileList, const QString &workingDirectory);
    void endOperation(bool success);

    void setSync(bool sync);

private:
    UnifiedDiffEditorWidget *m_widget = nullptr;
};

class SideBySideView : public IDiffView
{
    Q_OBJECT

public:
    SideBySideView();

    QWidget *widget();
    TextEditor::TextEditorWidget *leftEditorWidget();
    TextEditor::TextEditorWidget *rightEditorWidget();

    void setDocument(DiffEditorDocument *document);

    void beginOperation();
    void setCurrentDiffFileIndex(int index);
    void setDiff(const QList<FileData> &diffFileList, const QString &workingDirectory);
    void endOperation(bool success);

    void setSync(bool sync);

private:
    SideBySideDiffEditorWidget *m_widget;
};

} // namespace Internal
} // namespace DiffEditor
