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

#ifndef MANAGEDEFINITIONSDIALOG_H
#define MANAGEDEFINITIONSDIALOG_H

#include "ui_managedefinitionsdialog.h"
#include "highlightdefinitionmetadata.h"

#include <QList>

namespace TextEditor {
namespace Internal {

class ManageDefinitionsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ManageDefinitionsDialog(const QList<HighlightDefinitionMetaData> &metaDataList,
                                     const QString &path,
                                     QWidget *parent = 0);

protected:
    void changeEvent(QEvent *e);

private slots:
    void downloadDefinitions();
    void selectAll();
    void clearSelection();
    void invertSelection();

private:
    void populateDefinitionsWidget();

    QList<HighlightDefinitionMetaData> m_definitionsMetaData;
    QString m_path;
    Ui::ManageDefinitionsDialog ui;
};

} // namespace Internal
} // namespace TextEditor

#endif // MANAGEDEFINITIONSDIALOG_H
