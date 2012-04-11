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
#include "qmljsinspectorconstants.h"

#include <debugger/debuggerconstants.h>
#include <extensionsystem/pluginmanager.h>
#include <coreplugin/icore.h>

#include <QHeaderView>
#include <QItemDelegate>
#include <QLineEdit>
#include <QDoubleValidator>
#include <QPainter>

// expression editor
#include <QContextMenuEvent>
#include <QVBoxLayout>

// context menu
#include <QAction>
#include <QMenu>

#include <utils/qtcassert.h>
#include <utils/savedaction.h>

const int PROPERTY_NAME_COLUMN = 0;
const int PROPERTY_TYPE_COLUMN = 1;
const int PROPERTY_VALUE_COLUMN = 2;

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
        if (index.column() != PROPERTY_VALUE_COLUMN)
            return 0;

        switch (m_treeWidget->getTypeFor(index.row())) {

        case QmlJSPropertyInspector::BooleanType: {
            // invert the bool, skip editor
            int objectId = m_treeWidget->getData(index.row(),
                                                 PROPERTY_NAME_COLUMN,
                                                 Qt::UserRole).toInt();
            QString propertyName
                    = m_treeWidget->getData(index.row(),
                                            PROPERTY_NAME_COLUMN,
                                            Qt::DisplayRole).toString();
            bool propertyValue
                    = m_treeWidget->getData(index.row(), PROPERTY_VALUE_COLUMN,
                                            Qt::DisplayRole).toBool();
            m_treeWidget->propertyValueEdited(objectId, propertyName,
                                              !propertyValue?"true":"false",
                                              true);
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
        QVariant data = m_treeWidget->getData(index.row(), PROPERTY_VALUE_COLUMN,
                                              Qt::DisplayRole);
        QLineEdit *lineEdit = static_cast<QLineEdit*>(editor);
        lineEdit->setText(data.toString());
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const
    {
        Q_UNUSED(model);

        int objectId = m_treeWidget->getData(index.row(), PROPERTY_NAME_COLUMN,
                                             Qt::UserRole).toInt();
        if (objectId == -1)
            return;

        QString propertyName = m_treeWidget->getData(index.row(),
                                                     PROPERTY_NAME_COLUMN,
                                                     Qt::DisplayRole).toString();
        QLineEdit *lineEdit = static_cast<QLineEdit*>(editor);
        QString propertyValue = lineEdit->text();
        m_treeWidget->propertyValueEdited(objectId, propertyName, propertyValue,
                                          true);
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
    , m_buttonBox(new QDialogButtonBox(QDialogButtonBox::Ok
                                       | QDialogButtonBox::Cancel))
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
        return QLatin1String("#")
                + QString("%1").arg(alphaValue, 2, 16, QChar('0'))
                + color.name().right(6) ;
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

void ColorChooserDialog::setItemData(int objectId, const QString &propertyName,
                                     const QString &colorName)
{
    m_debugId = objectId;
    m_paramName = propertyName;
    m_mainFrame->setColor(QColor(colorName));
}

void ColorChooserDialog::acceptColor(const QColor &color)
{
    QDialog::accept();
    emit dataChanged(m_debugId, m_paramName,
                     QChar('\"') + color.name() + QChar('\"'));
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

// *************************************************************************
//  QmlJSPropertyInspectorModel
// *************************************************************************
QmlJSPropertyInspectorModel::QmlJSPropertyInspectorModel()
    : QStandardItemModel()
    , m_contentsValid(false)
{
}

Qt::ItemFlags QmlJSPropertyInspectorModel::flags(const QModelIndex &index) const
{
    return m_contentsValid ? QStandardItemModel::flags(index) : Qt::ItemFlags();
}

QVariant QmlJSPropertyInspectorModel::headerData(
        int section, Qt::Orientation orient, int role) const
{
    if (orient == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case PROPERTY_NAME_COLUMN: return tr("Name");
        case PROPERTY_VALUE_COLUMN: return tr("Value");
        case PROPERTY_TYPE_COLUMN: return tr("Type");
        };
    }
    return QStandardItemModel::headerData(section, orient, role);
}

void QmlJSPropertyInspectorModel::setContentsValid(bool contentsValid)
{
    m_contentsValid = contentsValid;
}

bool QmlJSPropertyInspectorModel::contentsValid() const
{
    return m_contentsValid;
}

QmlJSPropertyInspector::QmlJSPropertyInspector(QWidget *parent)
    : Utils::BaseTreeView(parent)
{
    setItemDelegateForColumn(PROPERTY_VALUE_COLUMN,
                             new PropertyEditDelegate(this));

    setModel(&m_model);
    //Add an empty Row to make the headers visible!
    addRow(QString(), QString(), QString(), -1, false);

    m_adjustColumnsAction = new Utils::SavedAction(this);
    m_adjustColumnsAction->setText(tr("Always Adjust Column Widths to Contents"));
    m_adjustColumnsAction->setCheckable(true);
    m_adjustColumnsAction->setValue(false);
    m_adjustColumnsAction->setDefaultValue(false);
    m_adjustColumnsAction->setSettingsKey(QLatin1String(Constants::S_QML_INSPECTOR),
        QLatin1String(Constants::ALWAYS_ADJUST_COLUMNS_WIDTHS));
    readSettings();
    connect(Core::ICore::instance(),
            SIGNAL(saveSettingsRequested()), SLOT(writeSettings()));

    setAlwaysAdjustColumnsAction(m_adjustColumnsAction);

    QAction *act = qobject_cast<QAction *>(
                ExtensionSystem::PluginManager::instance()->getObjectByName(
                QLatin1String(Debugger::Constants::USE_ALTERNATING_ROW_COLORS)));
    if (act) {
        setAlternatingRowColors(act->isChecked());
        connect(act, SIGNAL(toggled(bool)),
                SLOT(setAlternatingRowColorsHelper(bool)));
    }
}

void QmlJSPropertyInspector::readSettings()
{
    QSettings *settings = Core::ICore::settings();
    m_adjustColumnsAction->readSettings(settings);
}

void QmlJSPropertyInspector::writeSettings() const
{
    QSettings *settings = Core::ICore::settings();
    m_adjustColumnsAction->writeSettings(settings);
}

void QmlJSPropertyInspector::addBaseContextActions(QMenu *menu)
{
    QAction *act = qobject_cast<QAction *>(
                ExtensionSystem::PluginManager::instance()->getObjectByName(
                QLatin1String(Debugger::Constants::SORT_STRUCT_MEMBERS)));
    if (act)
        menu->addAction(act);
    Utils::BaseTreeView::addBaseContextActions(menu);

    act = qobject_cast<QAction *>(
                    ExtensionSystem::PluginManager::instance()->getObjectByName(
                    QLatin1String(Debugger::Constants::SETTINGS_DIALOG)));
    if (act)
        menu->addAction(act);
}

void QmlJSPropertyInspector::clear()
{
    m_model.removeRows(0, m_model.rowCount());
    m_currentObjects.clear();
}

void QmlJSPropertyInspector::setContentsValid(bool contentsValid)
{
    m_model.setContentsValid(contentsValid);
}

bool QmlJSPropertyInspector::contentsValid() const
{
    return m_model.contentsValid();
}

void QmlJSPropertyInspector::setCurrentObjects(
        const QList<QmlDebugObjectReference> &objectList)
{
    if (objectList.isEmpty())
        return;

    clear();

    foreach (const QmlDebugObjectReference &obj, objectList) {
        m_currentObjects << obj.debugId();
        buildPropertyTree(obj);
    }
}

QVariant QmlJSPropertyInspector::getData(int row, int column, int role) const
{
    return m_model.data(m_model.index(row, column), role);
}

QmlJSPropertyInspector::PropertyType
QmlJSPropertyInspector::getTypeFor(int row) const
{
    return static_cast<QmlJSPropertyInspector::PropertyType>(
                m_model.data(m_model.index(row, PROPERTY_TYPE_COLUMN),
                             Qt::UserRole).toInt());
}

void QmlJSPropertyInspector::propertyValueChanged(int debugId,
                                                  const QByteArray &propertyName,
                                                  const QVariant &propertyValue)
{
    if (m_model.rowCount() == 0)
        return;

    QString propertyNameS = QString(propertyName);
    for (int i = 0; i < m_model.rowCount(); i++) {
        if (m_model.data(m_model.index(i, PROPERTY_NAME_COLUMN),
                         Qt::DisplayRole).toString() == propertyNameS &&
                m_model.data(m_model.index(i, PROPERTY_NAME_COLUMN),
                             Qt::UserRole).toInt() == debugId) {
            QString oldData = m_model.data(m_model.index(i, PROPERTY_VALUE_COLUMN),
                                           Qt::DisplayRole).toString();
            QString newData = propertyValue.toString();
            if (QString(propertyValue.typeName()) == "QColor")
                newData = extendedNameFromColor(propertyValue);
            if (oldData != newData) {
                m_model.setData(m_model.index(i, PROPERTY_VALUE_COLUMN), newData,
                                Qt::DisplayRole);
                m_model.item(i, PROPERTY_VALUE_COLUMN)->setToolTip(newData);
                m_model.item(i, PROPERTY_NAME_COLUMN)->setForeground(QBrush(Qt::red));
                m_model.item(i, PROPERTY_VALUE_COLUMN)->setForeground(QBrush(Qt::red));
                m_model.item(i, PROPERTY_TYPE_COLUMN)->setForeground(QBrush(Qt::red));
                if (getTypeFor(i) == QmlJSPropertyInspector::ColorType)
                    setColorIcon(i);
            }
            break;
        }
    }
}

void QmlJSPropertyInspector::propertyValueEdited(const int objectId,
                                                 const QString &propertyName,
                                                 const QString &propertyValue,
                                                 bool isLiteral)
{
    emit changePropertyValue(objectId, propertyName, propertyValue, isLiteral);
}

void QmlJSPropertyInspector::buildPropertyTree(const QmlDebugObjectReference &obj)
{
    // Strip off the misleading metadata
    QString objTypeName = obj.className();
    QString declarativeString("QDeclarative");
    if (objTypeName.startsWith(declarativeString)) {
        objTypeName = objTypeName.mid(declarativeString.length()).section('_',
                                                                          0, 0);
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

    foreach (const QmlDebugPropertyReference &prop, obj.properties()) {
        QString propertyName = prop.name();
        QString propertyValue = prop.value().toString();

        if (cleanPropertyValue(propertyValue).isEmpty())
            continue;

        if (prop.valueTypeName() == "QColor") {
            propertyValue = extendedNameFromColor(prop.value());
        }

        addRow(propertyName, propertyValue, prop.valueTypeName(), obj.debugId(),
               prop.hasNotifySignal());
    }

    m_model.setHeaderData(PROPERTY_NAME_COLUMN, Qt::Horizontal,QVariant("name"));
    m_model.setHeaderData(PROPERTY_VALUE_COLUMN, Qt::Horizontal,QVariant("value"));
    m_model.setHeaderData(PROPERTY_TYPE_COLUMN, Qt::Horizontal,QVariant("type"));

    QAction *act = qobject_cast<QAction *>(
                ExtensionSystem::PluginManager::instance()->getObjectByName(
                QLatin1String(Debugger::Constants::SORT_STRUCT_MEMBERS)));
    if (act && act->isChecked())
        m_model.sort(PROPERTY_NAME_COLUMN);
}

void QmlJSPropertyInspector::addRow(const QString &name,const QString &value,
                                    const QString &type, const int debugId,
                                    bool editable)
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
    QmlJSPropertyInspector::PropertyType typeCode
            = QmlJSPropertyInspector::OtherType;
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
    newRow << nameColumn << typeColumn << valueColumn;
    m_model.appendRow(newRow);

    if (typeCode == QmlJSPropertyInspector::ColorType)
        setColorIcon(m_model.indexFromItem(valueColumn).row());
}

void QmlJSPropertyInspector::setColorIcon(int row)
{
    QStandardItem *item = m_model.item(row, PROPERTY_VALUE_COLUMN);
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
        isEditable = m_model.item(itemIndex.row(), PROPERTY_VALUE_COLUMN)->isEditable();
        isColor = (getTypeFor(itemIndex.row()) == QmlJSPropertyInspector::ColorType);
    }

    QAction exprAction(tr("Enter expression"), this);
    if (isEditable)
        menu.addAction(&exprAction);

    QAction colorAction(tr("Choose color"), this);
    if (isColor)
        menu.addAction(&colorAction);
    addBaseContextActions(&menu);

    QAction *action = menu.exec(ev->globalPos());
    if (action == 0)
        return;

    if (action == &exprAction)
        openExpressionEditor(itemIndex);
    if (action == &colorAction)
        openColorSelector(itemIndex);
    handleBaseContextAction(action);
}

void QmlJSPropertyInspector::openExpressionEditor(const QModelIndex &itemIndex)
{
    const QString propertyName = getData(itemIndex.row(), PROPERTY_NAME_COLUMN,
                                         Qt::DisplayRole).toString();
    const QString dialogText = tr("JavaScript expression for %1").arg(propertyName);
    const int objectId = getData(itemIndex.row(), PROPERTY_NAME_COLUMN,
                                 Qt::UserRole).toInt();

    ExpressionEdit *expressionDialog = new ExpressionEdit(dialogText);
    expressionDialog->setItemData(objectId, propertyName);

    connect(expressionDialog, SIGNAL(dataChanged(int,QString,QString)),
            this, SLOT(propertyValueEdited(int,QString,QString)));

    expressionDialog->show();
}

void QmlJSPropertyInspector::openColorSelector(const QModelIndex &itemIndex)
{
    const QString propertyName = getData(itemIndex.row(), PROPERTY_NAME_COLUMN,
                                         Qt::DisplayRole).toString();
    const QString dialogText = tr("Color selection for %1").arg(propertyName);
    const int objectId = getData(itemIndex.row(), PROPERTY_NAME_COLUMN,
                                 Qt::UserRole).toInt();
    const QString propertyValue = getData(itemIndex.row(), PROPERTY_VALUE_COLUMN,
                                          Qt::DisplayRole).toString();

    ColorChooserDialog *colorDialog = new ColorChooserDialog(dialogText);
    colorDialog->setItemData(objectId, propertyName, propertyValue);

    connect(colorDialog, SIGNAL(dataChanged(int,QString,QString)),
            this, SLOT(propertyValueEdited(int,QString,QString)));

    colorDialog->show();
}

} // Internal
} // QmlJSInspector
