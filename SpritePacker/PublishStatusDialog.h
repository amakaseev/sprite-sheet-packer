#ifndef PUBLISHSTATUSDIALOG_H
#define PUBLISHSTATUSDIALOG_H

#include <QtWidgets>

namespace Ui {
class PublishStatusDialog;
}

class PublishStatusDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PublishStatusDialog(QWidget *parent = 0);
    ~PublishStatusDialog();

    void log(const QString& msg);
    void complete();

private:
    Ui::PublishStatusDialog *ui;
};

#endif // PUBLISHSTATUSDIALOG_H
