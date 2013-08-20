/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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

#ifndef CPPPREPROCESSORADDITIONWIDGET_H
#define CPPPREPROCESSORADDITIONWIDGET_H

#include <cpptools/cppmodelmanagerinterface.h>
#include <utils/tooltip/tooltip.h>

#include <QWidget>
#include <QVariantMap>
#include <QPointer>

namespace Ui {
class CppPreProcessorAdditionWidget;
}

namespace CppEditor {
namespace Internal {

class PreProcessorAdditionWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PreProcessorAdditionWidget(QWidget *parent = 0);
    ~PreProcessorAdditionWidget();
    Ui::CppPreProcessorAdditionWidget *ui;

signals:
    void finished();
};

class PreProcessorAdditionPopUp : public Utils::ToolTip
{
    Q_OBJECT

public:
    ~PreProcessorAdditionPopUp(){}
    static PreProcessorAdditionPopUp *instance();

    void show(QWidget *parent, const QList<CppTools::ProjectPart::Ptr> &projectPartAdditions);
    bool eventFilter(QObject *o, QEvent *event);

signals:
    void finished(const QByteArray &additionalDefines);

private slots:
    void textChanged();
    void finish();
    void projectChanged(int index);
    void apply();
    void cancel();

protected:
    explicit PreProcessorAdditionPopUp();

private:
    struct ProjectPartAddition {
        CppTools::ProjectPart::Ptr projectPart;
        QByteArray additionalDefines;
    };

    PreProcessorAdditionWidget* widget;
    QList<ProjectPartAddition> originalPartAdditions;
    QList<ProjectPartAddition> partAdditions;

};

} // namespace CPPEditor
} // namespace Internal

#endif // CPPPREPROCESSORADDITIONWIDGET_H
