/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
** Author: Andreas Hartmetz, KDAB (andreas.hartmetz@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef MEMCHECKERRORVIEW_H
#define MEMCHECKERRORVIEW_H

#include <QListView>

namespace Valgrind {
namespace Internal {

class ValgrindBaseSettings;

class MemcheckErrorView : public QListView
{
    Q_OBJECT

public:
    MemcheckErrorView(QWidget *parent = 0);
    ~MemcheckErrorView();

    // Reimplemented to connect delegate to connection model after it has
    // been set by superclass implementation.
    void setModel(QAbstractItemModel *model);

    void setDefaultSuppressionFile(const QString &suppFile);
    QString defaultSuppressionFile() const;
    ValgrindBaseSettings *settings() const { return m_settings; }

    void updateGeometries();

public slots:
    void settingsChanged(ValgrindBaseSettings *settings);
    void goNext();
    void goBack();

signals:
    void resized();

private slots:
    void resizeEvent(QResizeEvent *e);
    void contextMenuEvent(QContextMenuEvent *e);
    void suppressError();
    void setCurrentRow(int row);

private:
    int rowCount() const;
    int currentRow() const;

    QAction *m_copyAction;
    QAction *m_suppressAction;
    QString m_defaultSuppFile;
    ValgrindBaseSettings *m_settings;
};

} // namespace Internal
} // namespace Valgrind

#endif // MEMCHECKERRORVIEW_H
