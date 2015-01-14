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

#ifndef FILESSELECTIONWIZARDPAGE_H
#define FILESSELECTIONWIZARDPAGE_H

#include <QWizardPage>

QT_BEGIN_NAMESPACE
class QVBoxLayout;
class QLabel;
class QTreeView;
class QLineEdit;
QT_END_NAMESPACE

namespace ProjectExplorer { class SelectableFilesModel; }

namespace GenericProjectManager {
namespace Internal {

class GenericProjectWizardDialog;

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
    void createHideFileFilterControls(QVBoxLayout *layout);
    void createShowFileFilterControls(QVBoxLayout *layout);
    void createApplyButton(QVBoxLayout *layout);

    GenericProjectWizardDialog *m_genericProjectWizardDialog;
    ProjectExplorer::SelectableFilesModel *m_model;

    QLabel *m_hideFilesFilterLabel;
    QLineEdit *m_hideFilesfilterLineEdit;

    QLabel *m_showFilesFilterLabel;
    QLineEdit *m_showFilesfilterLineEdit;

    QPushButton *m_applyFilterButton;

    QTreeView *m_view;
    QLabel *m_label;
    bool m_finished;
};

} // namespace Internal
} // namespace GenericProjectManager

#endif // FILESSELECTIONWIZARDPAGE_H
