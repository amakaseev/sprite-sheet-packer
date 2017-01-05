#include "PreferencesDialog.h"
#include "ui_PreferencesDialog.h"

PreferencesDialog::PreferencesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PreferencesDialog)
{
    ui->setupUi(this);
    setWindowFlags((windowFlags() | Qt::CustomizeWindowHint) & ~Qt::WindowMaximizeButtonHint);

    QSettings settings;
    QString customFormatFolder = settings.value("Preferences/customFormatFolder").toString();
    ui->customFormatFolderEdit->setText(customFormatFolder);
}

PreferencesDialog::~PreferencesDialog() {
    delete ui;
}

void PreferencesDialog::on_toolButton_clicked() {
    QSettings settings;

    QString customFormatFolder = ui->customFormatFolderEdit->text();
    customFormatFolder = QFileDialog::getExistingDirectory(this,
                                                 tr("Custom format folder"),
                                                 customFormatFolder,
                                                 QFileDialog::DontResolveSymlinks);
    if (!customFormatFolder.isEmpty()) {
        ui->customFormatFolderEdit->setText(customFormatFolder);
    }
}

void PreferencesDialog::on_buttonBox_accepted() {
    QSettings settings;
    settings.setValue("Preferences/customFormatFolder", ui->customFormatFolderEdit->text());
}

void PreferencesDialog::on_resetAllPushButton_clicked() {
    if (QMessageBox::critical(this, "Reset all", "Do you want to reset SpriteSheet Packer?\n\nSpriteSheet Packer will be closed, and restart!", QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes) {
        QSettings settings;
        settings.clear();
        settings.sync();

        QProcess::startDetached(QApplication::applicationFilePath());
        QCoreApplication::quit();
    }
}
