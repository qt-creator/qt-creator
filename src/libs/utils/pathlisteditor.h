/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef PATHLISTEDITOR_H
#define PATHLISTEDITOR_H

#include "utils_global.h"

#include <QWidget>
#include <QStringList>

namespace Utils {

struct PathListEditorPrivate;

class QTCREATOR_UTILS_EXPORT PathListEditor : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QStringList pathList READ pathList WRITE setPathList DESIGNABLE true)
    Q_PROPERTY(QString fileDialogTitle READ fileDialogTitle WRITE setFileDialogTitle DESIGNABLE true)

public:
    explicit PathListEditor(QWidget *parent = 0);
    virtual ~PathListEditor();

    QString pathListString() const;
    QStringList pathList() const;
    QString fileDialogTitle() const;

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
    PathListEditorPrivate *d;
};

} // namespace Utils

#endif // PATHLISTEDITOR_H
