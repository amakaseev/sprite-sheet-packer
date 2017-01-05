#include "ContentProtectionDialog.h"
#include "ui_ContentProtectionDialog.h"

ContentProtectionDialog::ContentProtectionDialog(const QString& encryptionKey, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ContentProtectionDialog)
{
    ui->setupUi(this);
    setWindowFlags((windowFlags() | Qt::CustomizeWindowHint) & ~Qt::WindowMaximizeButtonHint);

    ui->encryptionKeyLineEdit->setText(encryptionKey);
}

ContentProtectionDialog::~ContentProtectionDialog()
{
    qDebug() << "ContentProtectionDialog::~ContentProtectionDialog";
    delete ui;
}

QString ContentProtectionDialog::encryptionKey() {
    return ui->encryptionKeyLineEdit->text();
}

void ContentProtectionDialog::on_generatePushButton_clicked() {
//    QString code;
//    QString letters = "0123456789abcdef";
//    while (code.length() < 32) {
//        code.append(letters[rand() % letters.length()]);
//    }
    QString code = QUuid::createUuid().toString();
    code.remove('-');
    code.remove('{');
    code.remove('}');
    ui->encryptionKeyLineEdit->setText(code);
}

void ContentProtectionDialog::on_clearPushButton_clicked() {
    ui->encryptionKeyLineEdit->setText("");
}

void ContentProtectionDialog::on_saveGlobalPushButton_clicked() {
    if (!ui->encryptionKeyLineEdit->text().isEmpty()) {
        QSettings settings;
        settings.setValue("globalEncryptionKey", ui->encryptionKeyLineEdit->text());
    }
}

void ContentProtectionDialog::on_useGlobalPushButton_clicked() {
    QSettings settings;
    ui->encryptionKeyLineEdit->setText(settings.value("globalEncryptionKey").toString());
}
