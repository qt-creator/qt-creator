/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
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

#ifndef QNX_INTERNAL_BARDESCRIPTOREDITORWIDGET_H
#define QNX_INTERNAL_BARDESCRIPTOREDITORWIDGET_H

#include "bardescriptordocument.h"

#include <utils/environment.h>

#include <QStackedWidget>

QT_BEGIN_NAMESPACE
class QItemSelection;
class QLabel;
class QLineEdit;
class QStandardItemModel;
class QStandardItem;
class QStringListModel;
QT_END_NAMESPACE

namespace Core {
class IEditor;
}

namespace Qnx {
namespace Internal {

namespace Ui {
class BarDescriptorEditorWidget;
}

class BarDescriptorEditor;
class BarDescriptorPermissionsModel;
class BarDescriptorQtAssetsModel;
class BarDescriptorQtAssetsProxyModel;

class BarDescriptorEditorWidget : public QStackedWidget
{
    Q_OBJECT

public:
    explicit BarDescriptorEditorWidget(QWidget *parent = 0);
    ~BarDescriptorEditorWidget();

    Core::IEditor *editor() const;

    // General
    QString packageId() const;
    void setPackageId(const QString &packageId);

    QString packageVersion() const;
    void setPackageVersion(const QString &packageVersion);

    QString packageBuildId() const;
    void setPackageBuildId(const QString &packageBuildId);

    QString author() const;
    void setAuthor(const QString &author);

    QString authorId() const;
    void setAuthorId(const QString &authorId);

    // Application
    QString orientation() const;
    void setOrientation(const QString &orientation);

    QString chrome() const;
    void setChrome(const QString &chrome);

    bool transparent() const;
    void setTransparent(bool transparent);

    void appendApplicationArgument(const QString &argument);
    QStringList applicationArguments() const;

    QStringList checkedPermissions() const;
    void checkPermission(const QString &identifier);

    QList<Utils::EnvironmentItem> environment() const;
    void appendEnvironmentItem(const Utils::EnvironmentItem &envItem);

    QString applicationName() const;
    void setApplicationName(const QString &applicationName);

    QString applicationDescription() const;
    void setApplicationDescription(const QString &applicationDescription);

    QString applicationIconFileName() const;
    void setApplicationIcon(const QString &iconPath);

    QStringList splashScreens() const;
    void appendSplashScreen(const QString &splashScreenPath);

    // Assets
    void addAsset(const BarDescriptorAsset &asset);
    QList<BarDescriptorAsset> assets() const;

    QString xmlSource() const;
    void setXmlSource(const QString &xmlSource);

    bool isDirty() const;
    void clear();

public slots:
    void setDirty(bool dirty = true);

signals:
    void changed();

private slots:
    void addNewAsset();
    void removeSelectedAsset();
    void updateEntryCheckState(QStandardItem *item);
    void addImageAsAsset(const QString &path);

    void setApplicationIconDelayed(const QString &iconPath);
    void setApplicationIconPreview(const QString &path);

    void appendSplashScreenDelayed(const QString &splashScreenPath);
    void browseForSplashScreen();
    void removeSelectedSplashScreen();
    void handleSplashScreenSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

private:
    BarDescriptorEditor *createEditor();

    void initGeneralPage();
    void clearGeneralPage();
    void initApplicationPage();
    void clearApplicationPage();
    void initAssetsPage();
    void clearAssetsPage();
    void initSourcePage();
    void clearSourcePage();

    bool hasAsset(const BarDescriptorAsset &asset);
    QString localAssetPathFromDestination(const QString &path);

    void setImagePreview(QLabel *previewLabel, const QString &path);

    mutable Core::IEditor *m_editor;

    bool m_dirty;

    // Application
    BarDescriptorPermissionsModel *m_permissionsModel;
    QStringListModel *m_splashScreenModel;

    // Assets
    QStandardItemModel *m_assetsModel;

    Ui::BarDescriptorEditorWidget *m_ui;
};


} // namespace Internal
} // namespace Qnx
#endif // QNX_INTERNAL_BARDESCRIPTOREDITORWIDGET_H
