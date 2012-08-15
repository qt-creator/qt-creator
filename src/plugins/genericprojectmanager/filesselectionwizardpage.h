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

#ifndef FILESSELECTIONWIZARDPAGE_H
#define FILESSELECTIONWIZARDPAGE_H

#include <QWizardPage>
#include <QLabel>
#include <QTreeView>

namespace GenericProjectManager {
namespace Internal {

class GenericProjectWizardDialog;
class SelectableFilesModel;

class FilesSelectionWizardPage : public QWizardPage
{
    Q_OBJECT

public:
    FilesSelectionWizardPage(GenericProjectWizardDialog *genericProjectWizard, QWidget *parent = 0);
    bool isComplete() const;
    void initializePage();
    void cleanupPage();
    QStringList selectedFiles() const;
    QStringList selectedPaths() const;

private slots:
    void applyFilter();
    void parsingProgress(const QString &text);
    void parsingFinished();

private:
    GenericProjectWizardDialog *m_genericProjectWizardDialog;
    SelectableFilesModel *m_model;
    QLabel *m_filterLabel;
    QLineEdit *m_filterLineEdit;
    QPushButton *m_applyFilterButton;
    QTreeView *m_view;
    QLabel *m_label;
    bool m_finished;
};

} // namespace Internal
} // namespace GenericProjectManager

#endif // FILESSELECTIONWIZARDPAGE_H
