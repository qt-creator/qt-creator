#ifndef CONTEXTPANETEXTWIDGET_H
#define CONTEXTPANETEXTWIDGET_H

#include <QWidget>
#include <QVariant>

QT_BEGIN_NAMESPACE
namespace Ui {
    class ContextPaneTextWidget;
}
QT_END_NAMESPACE

namespace QmlJS {
    class PropertyReader;
}

namespace QmlDesigner {

class BauhausColorDialog;

class ContextPaneTextWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ContextPaneTextWidget(QWidget *parent = 0);
    ~ContextPaneTextWidget();
    void setProperties(QmlJS::PropertyReader *propertyReader);
    void setVerticalAlignmentVisible(bool);
    void setStyleVisible(bool);

public slots:
    void onTextColorButtonToggled(bool);
    void onColorButtonToggled(bool);
    void onColorDialogApplied(const QColor &color);
    void onColorDialogCancled();
    void onFontSizeChanged(int value);
    void onFontFormatChanged();
    void onBoldCheckedChanged(bool value);
    void onItalicCheckedChanged(bool value);
    void onUnderlineCheckedChanged(bool value);
    void onStrikeoutCheckedChanged(bool value);
    void onCurrentFontChanged(const QFont &font);
    void onHorizontalAlignmentChanged();
    void onVerticalAlignmentChanged();
    void onStyleComboBoxChanged(const QString &style);


signals:
    void propertyChanged(const QString &, const QVariant &);
    void removeProperty(const QString &);
    void removeAndChangeProperty(const QString &, const QString &, const QVariant &);

protected:
    void changeEvent(QEvent *e);
    void timerEvent(QTimerEvent *event);

private:
    Ui::ContextPaneTextWidget *ui;
    QString m_verticalAlignment;
    QString m_horizontalAlignment;
    int m_fontSizeTimer;
};

} //QmlDesigner

#endif // CONTEXTPANETEXTWIDGET_H
