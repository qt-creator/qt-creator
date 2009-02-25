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

#ifndef QUICKOPENTOOLWINDOW_H
#define QUICKOPENTOOLWINDOW_H

#include "quickopenplugin.h"

#include <QtCore/QEvent>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
class QAction;
class QLabel;
class QLineEdit;
class QMenu;
class QTreeView;
QT_END_NAMESPACE

namespace Core {
    namespace Utils {
        class FancyLineEdit;
    }
}
namespace QuickOpen {
namespace Internal {

class QuickOpenModel;
class CompletionList;

class QuickOpenToolWindow
  : public QWidget
{
    Q_OBJECT

public:
    QuickOpenToolWindow(QuickOpenPlugin *qop);

    void updateFilterList();

    void show(const QString &text, int selectionStart = -1, int selectionLength = 0);

private slots:
    void textEdited(const QString &text);
    void acceptCurrentEntry();
    void filterSelected();
    void showConfigureDialog();

private:
    bool eventFilter(QObject *obj, QEvent *event);

    void showEvent(QShowEvent *e);
    void focusInEvent(QFocusEvent *e);

    bool isShowingTypeHereMessage() const;
    void showCompletionList();
    void updateCompletionList(const QString &text);
    QList<IQuickOpenFilter*> filtersFor(const QString &text, QString &searchText);

    QuickOpenPlugin *m_quickOpenPlugin;
    QuickOpenModel *m_quickOpenModel;

    CompletionList *m_completionList;
    QMenu *m_filterMenu;
    QAction *m_refreshAction;
    QAction *m_configureAction;
    Core::Utils::FancyLineEdit *m_fileLineEdit;
};

} // namespace Internal
} // namespace QuickOpen

#endif // QUICKOPENTOOLWINDOW_H
