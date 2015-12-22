#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QtWidgets>

namespace Ui {
class PreferencesDialog;
}

class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget *parent = 0);
    ~PreferencesDialog();

private slots:
    void on_toolButton_clicked();
    void on_buttonBox_accepted();
    void on_resetAllPushButton_clicked();
    
private:
    Ui::PreferencesDialog *ui;
};

#endif // PREFERENCESDIALOG_H
