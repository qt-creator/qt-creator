/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
***************************************************************************/
#ifndef QUICKOPENTOOLWINDOW_H
#define QUICKOPENTOOLWINDOW_H

#include <QtCore/QEvent>
#include <QtGui/QWidget>

#include "quickopenplugin.h"

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
