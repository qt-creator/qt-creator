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
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
** 
***************************************************************************/

#ifndef VALUEEDITOR_H
#define VALUEEDITOR_H

#include "namespace_global.h"
#include "ui_valueeditor.h"

#include <QtCore/QList>
#include <QtGui/QWidget>
#include <QtCore/QPointer>

QT_BEGIN_NAMESPACE
class ProBlock;
class ProVariable;
QT_END_NAMESPACE

namespace Qt4ProjectManager {
namespace Internal {

class ProEditorModel;
class ProItemInfoManager;

class ValueEditor : public QWidget, protected Ui::ValueEditor
{
    Q_OBJECT

public:
    ValueEditor(QWidget *parent = 0);
    ~ValueEditor();

    void initialize(ProEditorModel *model, ProItemInfoManager *infomanager);

public slots:
    void editIndex(const QModelIndex &index);

protected slots:
    void modelChanged(const QModelIndex &index);

    void addItem(QString value = QString());
    void removeItem();

    void updateItemList(const QModelIndex &item);
    void updateItemChanges(QListWidgetItem *item);

    void updateVariableId();
    void updateVariableId(int index);
    void updateVariableOp(int index);

    void updateItemId();
    void updateItemId(int index);

private:
    enum ItemEditType {
        SingleDefined   = 0,
        SingleUndefined = 1,
        MultiDefined    = 2,
        MultiUndefined  = 3
    };

    void hideVariable();
    void showVariable(bool advanced);
    void setItemEditType(ItemEditType type);
    void setDescription(ItemEditType type, const QString &header, const QString &desc = QString());

    void initialize();

    void showVariable(ProVariable *variable);
    void showScope(ProBlock *scope);
    void showOther(ProBlock *block);

    ItemEditType itemType(bool defined, bool multiple) const;
    QModelIndex findValueIndex(const QString &id) const;

protected:
    QPointer<ProEditorModel> m_model;

private:
    bool m_handleModelChanges;

    QModelIndex m_currentIndex;
    ProItemInfoManager *m_infomanager;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // VALUEEDITOR_H
