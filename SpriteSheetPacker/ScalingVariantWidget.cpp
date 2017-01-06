#include "ScalingVariantWidget.h"
#include "ui_ScalingVariantWidget.h"

ScalingVariantWidget::ScalingVariantWidget(QWidget *parent, const QString& name, float scale, int maxTextureSize, bool pow2, bool forceSquared):
    QWidget(parent),
    ui(new Ui::ScalingVariantWidget)
{
    ui->setupUi(this);

    ui->nameLineEdit->setText(name);
    ui->scaleComboBox->setCurrentText(QString::number(scale));
    ui->maxTextureSizeComboBox->setCurrentText(QString::number(maxTextureSize));
    ui->pow2CheckBox->setChecked(pow2);
    ui->forceSquaredCheckBox->setChecked(forceSquared);
}

ScalingVariantWidget::~ScalingVariantWidget() {
    delete ui;
}

void ScalingVariantWidget::setRemoveEnabled(bool enable) {
    ui->removePushButton->setVisible(enable);
}

QString ScalingVariantWidget::name() {
    return ui->nameLineEdit->text();
}

float ScalingVariantWidget::scale() {
    return ui->scaleComboBox->currentText().toFloat();
}

int ScalingVariantWidget::maxTextureSize() {
    return ui->maxTextureSizeComboBox->currentText().toInt();
}

bool ScalingVariantWidget::pow2() {
    return ui->pow2CheckBox->isChecked();
}

bool ScalingVariantWidget::forceSquared() {
    return ui->forceSquaredCheckBox->isChecked();
}

void ScalingVariantWidget::setPow2(bool enable) {
    ui->pow2CheckBox->setChecked(enable);
}

void ScalingVariantWidget::setEnabledPow2(bool enable) {
    ui->pow2CheckBox->setEnabled(enable);
}

void ScalingVariantWidget::setForceSquared(bool enable) {
    ui->forceSquaredCheckBox->setChecked(enable);
}

void ScalingVariantWidget::on_removePushButton_clicked() {
    emit remove();
}

void ScalingVariantWidget::on_scaleComboBox_editTextChanged(const QString &) {
    QPalette comboboxPalette = ui->scaleComboBox->palette();

    bool isValideScale;
    if (ui->scaleComboBox->currentText().toFloat(&isValideScale) <= 0) {
        isValideScale = false;
    }

    if (!isValideScale) {
        comboboxPalette.setColor(QPalette::Text, Qt::red);
    } else {
        comboboxPalette.setColor(QPalette::Text, Qt::black);
        emit valueChanged(true);
    }
    ui->scaleComboBox->setPalette(comboboxPalette);
}

void ScalingVariantWidget::on_nameLineEdit_textChanged(const QString &) {
    emit valueChanged(false);
}

void ScalingVariantWidget::on_maxTextureSizeComboBox_currentTextChanged(const QString &text) {
    bool isValidSize;
    if (text.toInt(&isValidSize) > 8192) {
        isValidSize = false;
    }

    QPalette comboboxPalette = ui->maxTextureSizeComboBox->palette();
    if (!isValidSize) {
        comboboxPalette.setColor(QPalette::Text, Qt::red);
    } else {
        comboboxPalette.setColor(QPalette::Text, Qt::black);
        emit valueChanged(true);
    }
    ui->maxTextureSizeComboBox->setPalette(comboboxPalette);
}

void ScalingVariantWidget::on_pow2CheckBox_toggled(bool) {
    emit valueChanged(true);
}

void ScalingVariantWidget::on_forceSquaredCheckBox_toggled(bool) {
    emit valueChanged(true);
}
