/**************************************************************************
**
** This file is part of Qt Creator Instrumentation Tools
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
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

#ifndef ANALYZER_VALGRINDCONFIGWIDGET_H
#define ANALYZER_VALGRINDCONFIGWIDGET_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QStandardItemModel;
QT_END_NAMESPACE

namespace Valgrind {
namespace Internal {

namespace Ui {
class ValgrindConfigWidget;
}

class ValgrindBaseSettings;

class ValgrindConfigWidget : public QWidget
{
    Q_OBJECT

public:
    ValgrindConfigWidget(ValgrindBaseSettings *settings, QWidget *parent, bool global);
    virtual ~ValgrindConfigWidget();

    void setSuppressions(const QStringList &files);
    QStringList suppressions() const;

public Q_SLOTS:
    void slotAddSuppression();
    void slotRemoveSuppression();
    void slotSuppressionsRemoved(const QStringList &files);
    void slotSuppressionsAdded(const QStringList &files);
    void slotSuppressionSelectionChanged();

private slots:
    void updateUi();

private:
    ValgrindBaseSettings *m_settings;
    Ui::ValgrindConfigWidget *m_ui;
    QStandardItemModel *m_model;
};

} // namespace Internal
} // namespace Valgrind

#endif // ANALYZER_VALGRINDCONFIGWIDGET_H
