/****************************************************************************
**
** Copyright (C) 2013 Mateusz Łoskot <mateusz@loskot.net>
**
** This file, as part of Qt Creator Plugin for Boost.Build,
** was modified to accommodate OpenProjectWizard requirements.
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
** General Public License Version 2.1 by the Free Software
** Foundation and appearing in the file LICENSE.txt included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License Version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef FILESSELECTIONWIZARDPAGE_HPP
#define FILESSELECTIONWIZARDPAGE_HPP

#include <QWizardPage>

QT_BEGIN_NAMESPACE
class QVBoxLayout;
class QLabel;
class QTreeView;
class QLineEdit;
QT_END_NAMESPACE

namespace Utils { class PathChooser; }

namespace BoostBuildProjectManager {
namespace Internal {

class OpenProjectWizardDialog;
class SelectableFilesModel;

// TODO: Submit feature request to Qt Creator to make the FileSelectionWizardPage
//       a reusable component.
class FilesSelectionWizardPage : public QWizardPage
{
    Q_OBJECT

public:
    FilesSelectionWizardPage(OpenProjectWizardDialog *openProjectWizard, QWidget *parent = 0);
    bool isComplete() const;
    void initializePage();
    void cleanupPage();
    QStringList selectedFiles() const;
    QStringList selectedPaths() const;

private slots:
    void applyFilter();
    void parsingProgress(const QString &text);
    void parsingFinished();
    void baseDirectoryChanged();

private:
    void createHideFileFilterControls(QVBoxLayout *layout);
    void createShowFileFilterControls(QVBoxLayout *layout);
    void createChooseBaseDirControls(QVBoxLayout *layout);
    void createApplyButton(QVBoxLayout *layout);

    OpenProjectWizardDialog *m_openProjectWizardDialog;
    SelectableFilesModel *m_model;

    QLabel *m_hideFilesFilterLabel;
    QLineEdit *m_hideFilesfilterLineEdit;

    QLabel *m_showFilesFilterLabel;
    QLineEdit *m_showFilesfilterLineEdit;

    QPushButton *m_applyFilterButton;

    QLabel *m_baseDirLabel;
    Utils::PathChooser *m_baseDirChooser;
    QString m_lastBaseDir;

    QTreeView *m_view;
    QLabel *m_label;
    bool m_finished;
};

} // namespace Internal
} // namespace BoostBuildProjectManager

#endif // FILESSELECTIONWIZARDPAGE_HPP
