/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Andreas Hartmetz, KDAB (andreas.hartmetz@kdab.com)
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

#ifndef MEMCHECKERRORVIEW_H
#define MEMCHECKERRORVIEW_H

#include <QListView>

namespace Analyzer { class AnalyzerSettings; }

namespace Valgrind {
namespace Internal {

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
    Analyzer::AnalyzerSettings *settings() const { return m_settings; }

public slots:
    void settingsChanged(Analyzer::AnalyzerSettings *settings);
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
    Analyzer::AnalyzerSettings *m_settings;
};

} // namespace Internal
} // namespace Valgrind

#endif // MEMCHECKERRORVIEW_H
