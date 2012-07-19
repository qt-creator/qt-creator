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

#ifndef SCREENSHOTCROPPERWINDOW_H
#define SCREENSHOTCROPPERWINDOW_H

#include <QMainWindow>
#include "screenshotcropper.h"

using namespace QtSupport::Internal;

namespace Ui {
class ScreenShotCropperWindow;
}

class ScreenShotCropperWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ScreenShotCropperWindow(QWidget *parent = 0);
    ~ScreenShotCropperWindow();

    void loadData(const QString &areasXmlFile, const QString &imagesFolder);

public slots:
    void selectImage(int index);
    void setArea(const QRect &area);
    void saveData();

private:
    QMap<QString, QRect> m_areasOfInterest;
    QString m_areasOfInterestFile;
    QString m_imagesFolder;
    Ui::ScreenShotCropperWindow *ui;
};

#endif // SCREENSHOTCROPPERWINDOW_H
