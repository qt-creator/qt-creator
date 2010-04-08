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


#ifndef FILEWIDGET_H
#define FILEWIDGET_H

#include <QtGui/QWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QUrl>
#include <qmlitemnode.h>

QT_BEGIN_NAMESPACE

class FileWidget : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(QString fileName READ fileName WRITE setFileNameStr NOTIFY fileNameChanged)
    Q_PROPERTY(QString filter READ filter WRITE setFilter)
    Q_PROPERTY(bool showComboBox READ showComboBox WRITE setShowComboBox)
    Q_PROPERTY(QVariant itemNode READ itemNode WRITE setItemNode NOTIFY itemNodeChanged)        

public:

    FileWidget(QWidget *parent = 0);
    ~FileWidget();

    QVariant itemNode() const { return QVariant::fromValue(m_itemNode.modelNode()); }
    void setItemNode(const QVariant &itemNode);

    QString fileName() const
    { return m_fileName.toString(); }

    void setText(const QString &/*text*/)
    {

    }

    QString text() const
    {
        return QString();
    }

    void setFilter(const QString &filter)
    {
        m_filter = filter;
    }

    QString filter() const
    {
        return m_filter;
    }

    void setShowComboBox(bool show);

    bool showComboBox() const
    { return m_showComboBox; }

public slots:
    void setFileName(const QUrl &fileName);
    void setFileNameStr(const QString &fileName);
    void buttonPressed();
    void lineEditChanged();
    void comboBoxChanged();

signals:
    void fileNameChanged(const QUrl &fileName);
    void itemNodeChanged();

protected:

private:

    void setupComboBox();

    QPushButton *m_pushButton;
    QLineEdit *m_lineEdit;
    QComboBox *m_comboBox;
    QUrl m_fileName;
    QmlDesigner::QmlItemNode m_itemNode;
    QString m_filter;
    bool m_showComboBox;
    bool m_lock;

};

QT_END_NAMESPACE

#endif

