/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef PROJECTLOADWIZARD_H
#define PROJECTLOADWIZARD_H

#include "qt4project.h"

#include <QtGui/QWizard>

QT_BEGIN_NAMESPACE
class QWizardPage;
class QVBoxLayout;
class QHBoxLayout;
class QLabel;
class QCheckBox;
class QRadioButton;
class QListWidget;
class QLineEdit;
class QToolButton;
class QSpacerItem;
class QFormLayout;
class QComboBox;
QT_END_NAMESPACE

namespace Qt4ProjectManager {
class Qt4Project;
namespace Internal {

class ProjectLoadWizard : public QWizard
{
    Q_OBJECT
public:
    ProjectLoadWizard(Qt4ProjectManager::Qt4Project *project, QWidget * parent = 0, Qt::WindowFlags flags = 0);
    virtual ~ProjectLoadWizard();
    virtual int nextId() const;
    virtual void done(int result);
    void execDialog();

private:
    void addBuildConfiguration(QString name, QtVersion *qtversion, QtVersion::QmakeBuildConfig buildConfiguration);
    void setupImportPage(QtVersion *version, QtVersion::QmakeBuildConfig buildConfig);

    Qt4Project *m_project;

    // Only used for imported stuff
    QtVersion *m_importVersion;
    QtVersion::QmakeBuildConfig m_importBuildConfig;
    // Those that we might add
    bool m_temporaryVersion;

    // This was a file autogenarated by Designer, before I found out you can't actually
    // create non linear wizards in it
    // So those variables should all be m_*, but that one has to wait for refactoring support :)
    QWizardPage *importPage;
    QLabel *importLabel;
    QLabel *import2Label;
    QCheckBox *importCheckbox;

    void setupUi();
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // PROJECTLOADWIZARD_H
