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

#ifndef ENVIRONMENTEDITMODEL_H
#define ENVIRONMENTEDITMODEL_H

#include "environment.h"

#include <QtCore/QString>
#include <QtCore/QAbstractTableModel>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
class QTableView;
class QPushButton;
QT_END_NAMESPACE

namespace Utils {
class DetailsWidget;
}

namespace ProjectExplorer {

class EnvironmentModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    EnvironmentModel();
    ~EnvironmentModel();

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    QModelIndex addVariable();
    QModelIndex addVariable(const EnvironmentItem &item);
    void resetVariable(const QString &name);
    void unsetVariable(const QString &name);
    bool canUnset(const QString &name);
    bool canReset(const QString &name);
    QString indexToVariable(const QModelIndex &index) const;
    QModelIndex variableToIndex(const QString &name) const;
    bool changes(const QString &key) const;
    void setBaseEnvironment(const ProjectExplorer::Environment &env);
    QList<EnvironmentItem> userChanges() const;
    void setUserChanges(QList<EnvironmentItem> list);

signals:
    void userChangesChanged();
    /// Hint to the view where it should make sense to focus on next
    // This is a hack since there is no way for a model to suggest
    // the next interesting place to focus on to the view.
    void focusIndex(const QModelIndex &index);

private:
    void updateResultEnvironment();
    int findInChanges(const QString &name) const;
    int findInResultInsertPosition(const QString &name) const;
    int findInResult(const QString &name) const;

    ProjectExplorer::Environment m_baseEnvironment;
    ProjectExplorer::Environment m_resultEnvironment;
    QList<EnvironmentItem> m_items;
};

class PROJECTEXPLORER_EXPORT EnvironmentWidget : public QWidget
{
    Q_OBJECT
public:
    EnvironmentWidget(QWidget *parent, QWidget *additionalDetailsWidget = 0);
    ~EnvironmentWidget();

    void setBaseEnvironmentText(const QString &text);
    void setBaseEnvironment(const ProjectExplorer::Environment &env);

    QList<EnvironmentItem> userChanges() const;
    void setUserChanges(QList<EnvironmentItem> list);


signals:
    void userChangesChanged();
    void detailsVisibleChanged(bool visible);

private slots:
    void editEnvironmentButtonClicked();
    void addEnvironmentButtonClicked();
    void removeEnvironmentButtonClicked();
    void unsetEnvironmentButtonClicked();
    void environmentCurrentIndexChanged(const QModelIndex &current);
    void invalidateCurrentIndex();
    void updateSummaryText();
    void focusIndex(const QModelIndex &index);
    void updateButtons();

private:
    EnvironmentModel *m_model;
    QString m_baseEnvironmentText;
    Utils::DetailsWidget *m_detailsContainer;
    QTableView *m_environmentView;
    QPushButton *m_editButton;
    QPushButton *m_addButton;
    QPushButton *m_resetButton;
    QPushButton *m_unsetButton;
};

} // namespace ProjectExplorer

#endif // ENVIRONMENTEDITMODEL_H
