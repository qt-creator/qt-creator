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

#ifndef GITEDITOR_H
#define GITEDITOR_H

#include <vcsbase/vcsbaseeditor.h>

#include <QRegExp>

QT_BEGIN_NAMESPACE
class QVariant;
QT_END_NAMESPACE

namespace Git {
namespace Internal {

class GitEditorWidget : public VcsBase::VcsBaseEditorWidget
{
    Q_OBJECT

public:
    GitEditorWidget();

    void setPlainText(const QString &text) override;

private slots:
    void checkoutChange();
    void cherryPickChange();
    void revertChange();
    void logChange();
    void applyDiffChunk(const VcsBase::DiffChunk& chunk, bool revert);

private:
    void init() override;
    void resetChange(const QByteArray &resetType);
    void addDiffActions(QMenu *menu, const VcsBase::DiffChunk &chunk) override;
    void aboutToOpen(const QString &fileName, const QString &realFileName) override;
    QSet<QString> annotationChanges() const override;
    QString changeUnderCursor(const QTextCursor &) const override;
    VcsBase::BaseAnnotationHighlighter *createAnnotationHighlighter(const QSet<QString> &changes) const override;
    QString decorateVersion(const QString &revision) const override;
    QStringList annotationPreviousVersions(const QString &revision) const override;
    bool isValidRevision(const QString &revision) const override;
    void addChangeActions(QMenu *menu, const QString &change) override;
    QString revisionSubject(const QTextBlock &inBlock) const override;
    bool supportChangeLinks() const override;
    QString fileNameForLine(int line) const override;
    QString sourceWorkingDirectory() const;

    mutable QRegExp m_changeNumberPattern;
    QString m_currentChange;
};

} // namespace Git
} // namespace Internal

#endif // GITEDITOR_H
