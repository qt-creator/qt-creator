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

#ifndef QNX_INTERNAL_BARDESCRIPTOREDITORENTRYPOINTWIDGET_H
#define QNX_INTERNAL_BARDESCRIPTOREDITORENTRYPOINTWIDGET_H

#include "bardescriptoreditorabstractpanelwidget.h"

#include <QStandardItemModel>

QT_BEGIN_NAMESPACE
class QItemSelection;
class QLabel;
class QStringListModel;
QT_END_NAMESPACE

namespace Qnx {
namespace Internal {

namespace Ui {
class BarDescriptorEditorEntryPointWidget;
}

class BarDescriptorEditorEntryPointWidget : public BarDescriptorEditorAbstractPanelWidget
{
    Q_OBJECT

public:
    explicit BarDescriptorEditorEntryPointWidget(QWidget *parent = 0);
    ~BarDescriptorEditorEntryPointWidget();

    void clear();

    QString applicationName() const;
    void setApplicationName(const QString &applicationName);

    QString applicationDescription() const;
    void setApplicationDescription(const QString &applicationDescription);

    QString applicationIconFileName() const;
    void setApplicationIcon(const QString &iconPath);

    QStringList splashScreens() const;
    void appendSplashScreen(const QString &splashScreenPath);

    void setAssetsModel(QStandardItemModel *assetsModel);

signals:
    void imageAdded(const QString &path);
    void imageRemoved(const QString &path);

private slots:
    void setApplicationIconDelayed(const QString &iconPath);
    void setApplicationIconPreview(const QString &path);
    void validateIconSize(const QString &path);
    void handleIconChanged(const QString &path);
    void clearIcon();

    void browseForSplashScreen();
    void removeSelectedSplashScreen();
    void handleSplashScreenSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void appendSplashScreenDelayed(const QString &splashScreenPath);

private:
    enum ImageValidationResult {
        Valid,
        CouldNotLoad,
        IncorrectSize
    };

    void setImagePreview(QLabel *previewLabel, const QString &path);
    void validateSplashScreenSize(const QString &path);
    void validateImage(const QString &path, QLabel *warningMessage, QLabel *warningPixmap, const QSize &maximumSize);

    QString localAssetPathFromDestination(const QString &path);

    QStringListModel *m_splashScreenModel;
    QWeakPointer<QStandardItemModel> m_assetsModel;

    QString m_prevIconPath;

    Ui::BarDescriptorEditorEntryPointWidget *m_ui;
};


} // namespace Internal
} // namespace Qnx
#endif // QNX_INTERNAL_BARDESCRIPTOREDITORENTRYPOINTWIDGET_H
