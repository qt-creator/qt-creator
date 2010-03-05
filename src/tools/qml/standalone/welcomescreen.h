/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include <QtCore/QMetaType>
#include <QtGui/QDialog>
#include <private/qlistmodelinterface_p.h>

class QmlView;

class RecentFileModel : public QListModelInterface {
    Q_OBJECT
public:
    enum Roles {
        NameRole,
        FileNameRole
    };

    RecentFileModel(QObject *parent = 0);
    void setRecentFiles(const QStringList &filePaths);

    int count() const;
    QHash<int, QVariant> data(int index, const QList<int> &roles = QList<int>()) const;
    QVariant data(int index, int role) const;
    QList<int> roles() const;
    QString toString(int role) const;
private:
    QString displayPath(const QString &filePath) const;

    QStringList m_paths;
};

class WelcomeScreen : public QWidget {
    Q_OBJECT

public:
    WelcomeScreen(QWidget *parent);
    void setRecentFiles(const QStringList &recentFiles);

signals:
    void appExit();
    void openFile(const QString &filePath);
    void newFile(const QString &templatePath);

private slots:
    void openFile();

private:
    QmlView *m_view;
    RecentFileModel *m_recentFileModel;
};

