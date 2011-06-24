/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef TARGETSETUPPAGE_H
#define TARGETSETUPPAGE_H

#include "../qt4target.h"
#include "../qt4projectmanager_global.h"
#include <qtsupport/qtversionmanager.h>

#include <QtCore/QString>
#include <QtGui/QWizard>


QT_BEGIN_NAMESPACE
class QLabel;
class QMenu;
class QPushButton;
class QSpacerItem;
class QTreeWidget;
class QTreeWidgetItem;
class QVBoxLayout;
QT_END_NAMESPACE

namespace Qt4ProjectManager {
class Qt4Project;

namespace Internal {
namespace Ui {
class TargetSetupPage;
}
}

/// \internal
class QT4PROJECTMANAGER_EXPORT TargetSetupPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit TargetSetupPage(QWidget* parent = 0);
    ~TargetSetupPage();

    /// Initializes the TargetSetupPage
    /// \note The import information is gathered in initializePage(), make sure that the right proFilePath is set before
    void initializePage();
    /// Changes the default set of checked targets.
    /// call this before \sa initializePage()
    void setPreferredFeatures(const QSet<QString> &featureIds);
    /// Sets the features a target must support
    /// call this before \sa initializePage()
    void setRequiredFeatures(const QSet<QString> &featureIds);
    /// Sets the minimum qt version
    /// calls this before \sa initializePage()
    void setMinimumQtVersion(const QtSupport::QtVersionNumber &number);
    /// Sets whether the TargetSetupPage looks on disk for builds of this project
    /// call this before \sa initializePage()
    void setImportSearch(bool b);
    bool isComplete() const;
    bool setupProject(Qt4ProjectManager::Qt4Project *project);
    bool isTargetSelected(const QString &id) const;
    void setProFilePath(const QString &dir);

private slots:
    void newImportBuildConfiguration(const BuildConfigurationInfo &info);

private:
    void setupImportInfos();
    void cleanupImportInfos();
    void setupWidgets();
    void deleteWidgets();

    QSet<QString> m_preferredFeatures;
    QSet<QString> m_requiredFeatures;
    bool m_importSearch;
    QtSupport::QtVersionNumber m_minimumQtVersionNumber;
    QString m_proFilePath;
    QString m_defaultShadowBuildLocation;
    QMap<QString, Qt4TargetSetupWidget *> m_widgets;
    QHash<Qt4TargetSetupWidget *, Qt4BaseTargetFactory *> m_factories;

    QVBoxLayout *m_layout;
    QSpacerItem *m_spacer;
    Internal::Ui::TargetSetupPage *m_ui;
    QList<BuildConfigurationInfo> m_importInfos;
};

} // namespace Qt4ProjectManager

#endif // TARGETSETUPPAGE_H
