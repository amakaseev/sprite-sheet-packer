#include "ScalingVariantWidget.h"
#include "ui_ScalingVariantWidget.h"

ScalingVariantWidget::ScalingVariantWidget(QWidget *parent, const QString& folderName, float scale) :
    QWidget(parent),
    ui(new Ui::ScalingVariantWidget)
{
    ui->setupUi(this);

    ui->variantNameLineEdit->setText(folderName);
    ui->scaleComboBox->setCurrentText(QString::number(scale));
}

ScalingVariantWidget::~ScalingVariantWidget() {
    delete ui;
}

void ScalingVariantWidget::setRemoveEnabled(bool enable) {
    ui->removePushButton->setVisible(enable);
}

bool ScalingVariantWidget::isValideScale() {
    bool ok;
    ui->scaleComboBox->currentText().toFloat(&ok);
    return ok;
}

QString ScalingVariantWidget::variantFolder() {
    return ui->variantNameLineEdit->text();
}

float ScalingVariantWidget::scale() {
    return ui->scaleComboBox->currentText().toFloat();
}

void ScalingVariantWidget::on_removePushButton_clicked() {
    emit remove();
}

void ScalingVariantWidget::on_scaleComboBox_editTextChanged(const QString &arg1) {
    QPalette comboboxPalette = ui->scaleComboBox->palette();
    if (!isValideScale()) {
        comboboxPalette.setColor(QPalette::Text, Qt::red);
    } else {
        comboboxPalette.setColor(QPalette::Text, Qt::black);
    }
    ui->scaleComboBox->setPalette(comboboxPalette);
}
