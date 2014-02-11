/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef DIFFEDITORCONTROLLER_H
#define DIFFEDITORCONTROLLER_H

#include "diffeditor_global.h"

#include <QObject>

namespace DiffEditor {

class DIFFEDITOR_EXPORT DiffEditorController : public QObject
{
    Q_OBJECT
public:
    class DiffFileInfo {
    public:
        DiffFileInfo() {}
        DiffFileInfo(const QString &file) : fileName(file) {}
        DiffFileInfo(const QString &file, const QString &type) : fileName(file), typeInfo(type) {}
        QString fileName;
        QString typeInfo;
    };

    class DiffFilesContents {
    public:
        DiffFileInfo leftFileInfo;
        QString leftText;
        DiffFileInfo rightFileInfo;
        QString rightText;
    };

    DiffEditorController(QObject *parent = 0);
    ~DiffEditorController();

    QString clearMessage() const;

    QList<DiffFilesContents> diffContents() const;
    QString workingDirectory() const;
    QString description() const;
    bool isDescriptionEnabled() const;

    bool isDescriptionVisible() const;
    int contextLinesNumber() const;
    bool isIgnoreWhitespaces() const;
    bool horizontalScrollBarSynchronization() const;
    int currentDiffFileIndex() const;

public slots:
    void clear();
    void clear(const QString &message);
    void setDiffContents(const QList<DiffEditorController::DiffFilesContents> &diffFileList,
                         const QString &workingDirectory = QString());
    void setDescription(const QString &description);
    void setDescriptionEnabled(bool on);

    void setDescriptionVisible(bool on);
    void setContextLinesNumber(int lines);
    void setIgnoreWhitespaces(bool ignore);
    void setHorizontalScrollBarSynchronization(bool on);
    void setCurrentDiffFileIndex(int diffFileIndex);

signals:
    // This sets the current diff file index to -1
    void cleared(const QString &message);
    // This sets the current diff file index to 0 (unless diffFileList is empty)
    void diffContentsChanged(const QList<DiffEditorController::DiffFilesContents> &diffFileList, const QString &workingDirectory);
    void descriptionChanged(const QString &description);
    void descriptionEnablementChanged(bool on);

    void descriptionVisibilityChanged(bool on);
    void contextLinesNumberChanged(int lines);
    void ignoreWhitespacesChanged(bool ignore);
    void horizontalScrollBarSynchronizationChanged(bool on);
    void currentDiffFileIndexChanged(int diffFileIndex);

private:
    QString m_clearMessage;

    QList<DiffFilesContents> m_diffFileList;
    QString m_workingDirectory;
    QString m_description;
    bool m_descriptionEnabled;

    bool m_descriptionVisible;
    int m_contextLinesNumber;
    bool m_ignoreWhitespaces;
    bool m_syncScrollBars;
    int m_currentDiffFileIndex;
};

} // namespace DiffEditor

#endif // DIFFEDITORCONTROLLER_H
