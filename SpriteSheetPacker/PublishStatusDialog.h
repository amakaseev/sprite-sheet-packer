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

public slots:
    void log(const QString& msg, const QColor& color = Qt::black);
    void complete();

private:
    Ui::PublishStatusDialog *ui;
};

#endif // PUBLISHSTATUSDIALOG_H
