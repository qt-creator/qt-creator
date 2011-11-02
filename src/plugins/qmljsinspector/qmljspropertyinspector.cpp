/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
#include "qmljspropertyinspector.h"

#include <QtGui/QHeaderView>
#include <QtGui/QItemDelegate>
#include <QtGui/QLineEdit>
#include <QtGui/QDoubleValidator>
#include <QtGui/QPainter>

// expression editor
#include <QtGui/QContextMenuEvent>
#include <QtGui/QVBoxLayout>

// context menu
#include <QtGui/QAction>
#include <QtGui/QMenu>

#include <utils/qtcassert.h>

namespace QmlJSInspector {
namespace Internal {

// *************************************************************************
//  PropertyEdit
// *************************************************************************

class PropertyEditDelegate : public QItemDelegate
{
    public:
    explicit PropertyEditDelegate(QObject *parent=0) : QItemDelegate(parent),
        m_treeWidget(dynamic_cast<QmlJSPropertyInspector *>(parent)) {}

    QWidget *createEditor(QWidget *parent,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const
    {
        Q_UNUSED(option);
        if (index.column() != 1)
            return 0;

        switch (m_treeWidget->getTypeFor(index.row())) {

            case QmlJSPropertyInspector::BooleanType: {
                // invert the bool, skip editor
                int objectId = m_treeWidget->getData(index.row(), 0, Qt::UserRole).toInt();
                QString propertyName = m_treeWidget->getData(index.row(), 0, Qt::DisplayRole).toString();
                bool propertyValue = m_treeWidget->getData(index.row(), 1, Qt::DisplayRole).toBool();
                m_treeWidget->propertyValueEdited(objectId, propertyName, !propertyValue?"true":"false");
                return 0;
            }

            case QmlJSPropertyInspector::NumberType: {
                QLineEdit *editor = new QLineEdit(parent);
                editor->setValidator(new QDoubleValidator(editor));
                return editor;
            }

        default: {
                return new QLineEdit(parent);
            }
        }

        return 0;
    }

    void setEditorData(QWidget *editor, const QModelIndex &index) const
    {
        QVariant data = m_treeWidget->getData(index.row(), 1, Qt::DisplayRole);
        QLineEdit *lineEdit = static_cast<QLineEdit*>(editor);
        lineEdit->setText(data.toString());
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
    {
        Q_UNUSED(model);

        int objectId = m_treeWidget->getData(index.row(), 0, Qt::UserRole).toInt();
        if (objectId == -1)
            return;

        QString propertyName = m_treeWidget->getData(index.row(), 0, Qt::DisplayRole).toString();

        QLineEdit *lineEdit = static_cast<QLineEdit*>(editor);
        QString propertyValue = lineEdit->text();

        // add quotes if it's a string
        QmlJSPropertyInspector::PropertyType propertyType = m_treeWidget->getTypeFor(index.row());
        const QChar quote(QLatin1Char('\"'));

        if (propertyType == QmlJSPropertyInspector::StringType) {
            const QChar backslash(QLatin1Char('\\'));
            propertyValue = propertyValue.replace(quote, QString(backslash) + quote);
        }

        if (propertyType == QmlJSPropertyInspector::StringType || propertyType == QmlJSPropertyInspector::ColorType) {
            propertyValue = quote + propertyValue + quote;
        }

        m_treeWidget->propertyValueEdited(objectId, propertyName, propertyValue);

        lineEdit->clearFocus();
    }

    void updateEditorGeometry(QWidget *editor,
                              const QStyleOptionViewItem &option,
                              const QModelIndex &index) const
    {
        Q_UNUSED(index);
        QLineEdit *lineEdit = static_cast<QLineEdit*>(editor);
        lineEdit->setGeometry(option.rect);
    }

private:
    QmlJSPropertyInspector *m_treeWidget;

};
// *************************************************************************
//  expressionEdit
// *************************************************************************

ExpressionEdit::ExpressionEdit(const QString &title, QDialog *parent)
    : QDialog(parent)
    , m_buttonBox(new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel))
    , m_exprInput(new QLineEdit(this))
{
    setWindowTitle(title);

    QVBoxLayout *vertLayout = new QVBoxLayout;
    m_exprInput->setMinimumWidth(550);
    connect(m_exprInput,SIGNAL(returnPressed()),this,SLOT(accept()));
    vertLayout->addWidget(m_exprInput);
    vertLayout->addWidget(m_buttonBox);
    setLayout(vertLayout);

    connect(m_buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

QString ExpressionEdit::expression() const
{
    return m_exprInput->text();
}

void ExpressionEdit::setItemData(int objectId, const QString &propertyName)
{
    m_debugId = objectId;
    m_paramName = propertyName;
}

void ExpressionEdit::accept()
{
    QDialog::accept();
    emit dataChanged(m_debugId, m_paramName, expression());
}

// *************************************************************************
//  color chooser
// *************************************************************************

inline QString extendedNameFromColor(QColor color)
{
    int alphaValue = color.alpha();
    if (alphaValue < 255)
        return QLatin1String("#") + QString("%1").arg(alphaValue, 2, 16, QChar('0')) + color.name().right(6) ;
    else
        return color.name();
}

inline QString extendedNameFromColor(QVariant color) {
    return extendedNameFromColor(QColor(color.value<QColor>()));
}

inline QColor colorFromExtendedName(QString name) {
    QRegExp validator("#([0-9a-f]{2})([0-9a-f]{2})([0-9a-f]{2})([0-9a-f]{2})");
    if (validator.exactMatch(name)) {
        return QColor(validator.cap(2).toInt(0,16),
                      validator.cap(3).toInt(0,16),
                      validator.cap(4).toInt(0,16),
                      validator.cap(1).toInt(0,16));
    }
    return QColor(name);
}

ColorChooserDialog::ColorChooserDialog(const QString &title, QDialog *parent)
    : QDialog(parent)
{
    setWindowTitle(title);

    QVBoxLayout *vertLayout = new QVBoxLayout;
    m_mainFrame = new QmlEditorWidgets::CustomColorDialog(this);
    setLayout(vertLayout);

    setFixedSize(m_mainFrame->size());

    connect(m_mainFrame,SIGNAL(accepted(QColor)),this,SLOT(acceptColor(QColor)));
    connect(m_mainFrame,SIGNAL(rejected()),this,SLOT(reject()));
}

void ColorChooserDialog::setItemData(int objectId, const QString &propertyName, const QString& colorName)
{
    m_debugId = objectId;
    m_paramName = propertyName;
    m_mainFrame->setColor(QColor(colorName));
}

void ColorChooserDialog::acceptColor(const QColor &color)
{
    QDialog::accept();
    emit dataChanged(m_debugId, m_paramName, QChar('\"')+color.name()+QChar('\"'));
}

// *************************************************************************
//  FILTER
// *************************************************************************
bool PropertiesFilter::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index0 = sourceModel()->index(sourceRow, 0, sourceParent);
    QModelIndex index1 = sourceModel()->index(sourceRow, 1, sourceParent);
    QModelIndex index2 = sourceModel()->index(sourceRow, 2, sourceParent);

    return (sourceModel()->data(index0).toString().contains(filterRegExp())
                    || sourceModel()->data(index1).toString().contains(filterRegExp())
                    || sourceModel()->data(index2).toString().contains(filterRegExp()));
}

// *************************************************************************
//  QmlJSObjectTree
// *************************************************************************
inline QString cleanPropertyValue(QString propertyValue)
{
    if (propertyValue == QString("<unknown value>"))
        return QString();
    if (propertyValue == QString("<unnamed object>"))
        return QString();
    return propertyValue;
}

QmlJSPropertyInspector::QmlJSPropertyInspector(QWidget *parent)
    : QTreeView(parent)
{
    setAttribute(Qt::WA_MacShowFocusRect, false);
    setFrameStyle(QFrame::NoFrame);
    setExpandsOnDoubleClick(true);

    header()->setResizeMode(QHeaderView::ResizeToContents);
    header()->setMinimumSectionSize(150);
    setRootIsDecorated(false);

    setItemDelegateForColumn(1, new PropertyEditDelegate(this));

    m_filter = new PropertiesFilter(this);
    m_filter->setSourceModel(&m_model);
    setModel(m_filter);
}

void QmlJSPropertyInspector::filterBy(const QString &expression)
{
    m_filter->setFilterWildcard(expression);
    m_filter->setFilterCaseSensitivity(Qt::CaseInsensitive);
}

void QmlJSPropertyInspector::clear()
{
    m_model.clear();
    m_currentObjects.clear();
}

void QmlJSPropertyInspector::setCurrentObjects(const QList<QDeclarativeDebugObjectReference> &objectList)
{
    if (objectList.isEmpty())
        return;

    clear();

    foreach (const QDeclarativeDebugObjectReference &obj, objectList) {
        m_currentObjects << obj.debugId();
        buildPropertyTree(obj);
    }
}

QVariant QmlJSPropertyInspector::getData(int row, int column, int role) const
{
    return m_filter->data(m_filter->index(row, column), role);
}

QmlJSPropertyInspector::PropertyType QmlJSPropertyInspector::getTypeFor(int row) const
{
    return static_cast<QmlJSPropertyInspector::PropertyType>(m_filter->data(m_filter->index(row,2),Qt::UserRole).toInt());
}

void QmlJSPropertyInspector::propertyValueChanged(int debugId, const QByteArray &propertyName, const QVariant &propertyValue)
{
    if (m_model.rowCount() == 0)
        return;

    QString propertyNameS = QString(propertyName);
    for (int i = 0; i < m_model.rowCount(); i++) {
        if (m_model.data(m_model.index(i, 0), Qt::DisplayRole).toString() == propertyNameS &&
                m_model.data(m_model.index(i, 0), Qt::UserRole).toInt() == debugId) {
            QString oldData = m_model.data(m_model.index(i, 1), Qt::DisplayRole).toString();
            QString newData = propertyValue.toString();
            if (QString(propertyValue.typeName()) == "QColor")
                newData = extendedNameFromColor(propertyValue);
            if (oldData != newData) {
                m_model.setData(m_model.index(i, 1), newData, Qt::DisplayRole);
                m_model.item(i, 1)->setToolTip(newData);
                m_model.item(i, 0)->setForeground(QBrush(Qt::red));
                m_model.item(i, 1)->setForeground(QBrush(Qt::red));
                m_model.item(i, 2)->setForeground(QBrush(Qt::red));
                if (getTypeFor(i) == QmlJSPropertyInspector::ColorType)
                    setColorIcon(i);
            }
            break;
        }
    }
}

void QmlJSPropertyInspector::propertyValueEdited(const int objectId,const QString& propertyName, const QString& propertyValue)
{
    emit changePropertyValue(objectId, propertyName, propertyValue);
}

void QmlJSPropertyInspector::buildPropertyTree(const QDeclarativeDebugObjectReference &obj)
{
    // Strip off the misleading metadata
    QString objTypeName = obj.className();
    QString declarativeString("QDeclarative");
    if (objTypeName.startsWith(declarativeString)) {
        objTypeName = objTypeName.mid(declarativeString.length()).section('_',0,0);
    }

    // class
    addRow(QString("class"),
           objTypeName,
           QString("qmlType"),
           obj.debugId(),
           false);

    // id
    if (!obj.idString().isEmpty()) {
        addRow(QString("id"),
               obj.idString(),
               QString("idString"),
               obj.debugId(),
               false);
    }

    foreach (const QDeclarativeDebugPropertyReference &prop, obj.properties()) {
        QString propertyName = prop.name();
        QString propertyValue = prop.value().toString();

        if (cleanPropertyValue(propertyValue).isEmpty())
            continue;

        if (prop.valueTypeName() == "QColor") {
            propertyValue = extendedNameFromColor(prop.value());
        }

        addRow(propertyName, propertyValue, prop.valueTypeName(), obj.debugId(), prop.hasNotifySignal());
    }

    m_model.setHeaderData(0,Qt::Horizontal,QVariant("name"));
    m_model.setHeaderData(1,Qt::Horizontal,QVariant("value"));
    m_model.setHeaderData(2,Qt::Horizontal,QVariant("type"));

}

void QmlJSPropertyInspector::addRow(const QString &name,const QString &value, const QString &type,
                             const int debugId, bool editable)
{
    QStandardItem *nameColumn = new QStandardItem(name);
    nameColumn->setToolTip(name);
    nameColumn->setData(QVariant(debugId),Qt::UserRole);
    nameColumn->setEditable(false);

    QStandardItem *valueColumn = new QStandardItem(value);
    valueColumn->setToolTip(value);
    valueColumn->setEditable(editable);
    valueColumn->setData(QVariant(editable),Qt::UserRole+1);

    QStandardItem *typeColumn = new QStandardItem(type);
    typeColumn->setToolTip(type);
    typeColumn->setEditable(false);

    // encode type for easy lookup
    QmlJSPropertyInspector::PropertyType typeCode = QmlJSPropertyInspector::OtherType;
    if (type == "bool")
        typeCode = QmlJSPropertyInspector::BooleanType;
    else if (type == "qreal")
        typeCode = QmlJSPropertyInspector::NumberType;
    else if (type == "QString")
        typeCode = QmlJSPropertyInspector::StringType;
    else if (type == "QColor")
        typeCode = QmlJSPropertyInspector::ColorType;

    typeColumn->setData(typeCode, Qt::UserRole);

    QList<QStandardItem *> newRow;
    newRow << nameColumn << valueColumn << typeColumn;
    m_model.appendRow(newRow);

    if (typeCode == QmlJSPropertyInspector::ColorType)
        setColorIcon(m_model.indexFromItem(valueColumn).row());
}

void QmlJSPropertyInspector::setColorIcon(int row)
{
    QStandardItem *item = m_model.itemFromIndex(m_model.index(row, 1));
    QColor color = colorFromExtendedName(item->data(Qt::DisplayRole).toString());

    int recomendedLength = viewOptions().decorationSize.height() - 2;

    QPixmap colorpix(recomendedLength, recomendedLength);
    QPainter p(&colorpix);
    if (color.alpha() != 255)
        p.fillRect(1,1, recomendedLength -2, recomendedLength - 2, Qt::white);
    p.fillRect(1, 1, recomendedLength - 2, recomendedLength - 2, color);
    p.setPen(Qt::black);
    p.drawRect(0, 0, recomendedLength - 1, recomendedLength - 1);
    item->setIcon(colorpix);
}

void QmlJSPropertyInspector::contextMenuEvent(QContextMenuEvent *ev)
{
    QMenu menu;
    QModelIndex itemIndex = indexAt(ev->pos());
    bool isEditable = false;
    bool isColor = false;
    if (itemIndex.isValid()) {
        isEditable = m_model.itemFromIndex(m_filter->mapToSource(m_filter->index(itemIndex.row(), 1)))->isEditable();
        isColor = (getTypeFor(itemIndex.row()) == QmlJSPropertyInspector::ColorType);
    }

    QAction exprAction(tr("Enter expression"), this);
    if (isEditable)
        menu.addAction(&exprAction);

    QAction colorAction(tr("Choose color"), this);
    if (isColor)
        menu.addAction(&colorAction);

    QAction *action = menu.exec(ev->globalPos());
    if (action == 0)
        return;

    if (action == &exprAction)
        openExpressionEditor(itemIndex);
    if (action == &colorAction)
        openColorSelector(itemIndex);
}

void QmlJSPropertyInspector::openExpressionEditor(const QModelIndex &itemIndex)
{
    const QString propertyName = getData(itemIndex.row(), 0, Qt::DisplayRole).toString();
    const QString dialogText = tr("JavaScript expression for %1").arg(propertyName);
    const int objectId = getData(itemIndex.row(), 0, Qt::UserRole).toInt();

    ExpressionEdit *expressionDialog = new ExpressionEdit(dialogText);
    expressionDialog->setItemData(objectId, propertyName);

    connect(expressionDialog, SIGNAL(dataChanged(int,QString,QString)),
            this, SLOT(propertyValueEdited(int,QString,QString)));

    expressionDialog->show();
}

void QmlJSPropertyInspector::openColorSelector(const QModelIndex &itemIndex)
{
    const QString propertyName = getData(itemIndex.row(), 0, Qt::DisplayRole).toString();
    const QString dialogText = tr("Color selection for %1").arg(propertyName);
    const int objectId = getData(itemIndex.row(), 0, Qt::UserRole).toInt();
    const QString propertyValue = getData(itemIndex.row(), 1, Qt::DisplayRole).toString();

    ColorChooserDialog *colorDialog = new ColorChooserDialog(dialogText);
    colorDialog->setItemData(objectId, propertyName, propertyValue);

    connect(colorDialog, SIGNAL(dataChanged(int,QString,QString)),
            this, SLOT(propertyValueEdited(int,QString,QString)));

    colorDialog->show();
}

} // Internal
} // QmlJSInspector
