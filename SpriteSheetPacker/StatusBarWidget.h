#ifndef STATUSBARWIDGET_H
#define STATUSBARWIDGET_H

#include <QtWidgets>

namespace Ui {
class StatusBarWidget;
}

class StatusBarWidget : public QWidget
{
    Q_OBJECT

public:
    explicit StatusBarWidget(QWidget *parent = 0);
    ~StatusBarWidget();

    void showMessage(const QString& message, const QPixmap& pixmap = QPixmap());
    void showSpinner(const QString& message);
    void hideSpinner();

private:
    Ui::StatusBarWidget *ui;
};

#endif // STATUSBARWIDGET_H
