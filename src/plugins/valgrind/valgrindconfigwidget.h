/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef ANALYZER_VALGRINDCONFIGWIDGET_H
#define ANALYZER_VALGRINDCONFIGWIDGET_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QStandardItemModel;
QT_END_NAMESPACE

namespace Valgrind {
namespace Internal {

namespace Ui { class ValgrindConfigWidget; }

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
