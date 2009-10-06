/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef PATHLISTEDITOR_H
#define PATHLISTEDITOR_H

#include "utils_global.h"

#include <QtGui/QWidget>
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace Utils {

struct PathListEditorPrivate;

/**
 * A control that let's the user edit a list of (directory) paths
 * using the platform separator (';',':'). Typically used for
 * path lists controlled by environment variables, such as
 * PATH. It is based on a QPlainTextEdit as it should
 * allow for convenient editing and non-directory type elements like
 * "etc/mydir1:$SPECIAL_SYNTAX:/etc/mydir2".
 * When pasting text into it, the platform separator will be replaced
 * by new line characters for convenience.
 */

class QTCREATOR_UTILS_EXPORT PathListEditor : public QWidget
{
    Q_DISABLE_COPY(PathListEditor)
    Q_OBJECT
    Q_PROPERTY(QStringList pathList READ pathList WRITE setPathList DESIGNABLE true)
    Q_PROPERTY(QString fileDialogTitle READ fileDialogTitle WRITE setFileDialogTitle DESIGNABLE true)

public:
    explicit PathListEditor(QWidget *parent = 0);
    virtual ~PathListEditor();

    QString pathListString() const;
    QStringList pathList() const;
    QString fileDialogTitle() const;

    static QChar separator();

    // Add a convenience action "Import from 'Path'" (environment variable)
    void addEnvVariableImportAction(const QString &var);

public slots:
    void clear();
    void setPathList(const QStringList &l);
    void setPathList(const QString &pathString);
    void setPathListFromEnvVariable(const QString &var);
    void setFileDialogTitle(const QString &l);

protected:
    // Index after which to insert further "Add" actions
    static int lastAddActionIndex();
    QAction *insertAction(int index /* -1 */, const QString &text, QObject * receiver, const char *slotFunc);
    QAction *addAction(const QString &text, QObject * receiver, const char *slotFunc);

    QString text() const;
    void setText(const QString &);

protected slots:
    void insertPathAtCursor(const QString &);
    void deletePathAtCursor();
    void appendPath(const QString &);

private slots:
    void slotAdd();
    void slotInsert();

private:
    PathListEditorPrivate *m_d;
};

} // namespace Utils

#endif // PATHLISTEDITOR_H
