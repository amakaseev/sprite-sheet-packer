#ifndef CONTENTPROTECTIONDIALOG_H
#define CONTENTPROTECTIONDIALOG_H

#include <QtWidgets>

namespace Ui {
class ContentProtectionDialog;
}

class ContentProtectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ContentProtectionDialog(const QString& encryptionKey, QWidget *parent = 0);
    ~ContentProtectionDialog();

    QString encryptionKey();

private slots:
    void on_generatePushButton_clicked();
    void on_clearPushButton_clicked();
    void on_saveGlobalPushButton_clicked();
    void on_useGlobalPushButton_clicked();

private:
    Ui::ContentProtectionDialog *ui;
};

#endif // CONTENTPROTECTIONDIALOG_H
