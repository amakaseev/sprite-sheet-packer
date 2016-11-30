#ifndef SCALINGVARIANTWIDGET_H
#define SCALINGVARIANTWIDGET_H

#include <QWidget>

namespace Ui {
class ScalingVariantWidget;
}

class ScalingVariantWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ScalingVariantWidget(QWidget *parent = 0, const QString& name = "", float scale = 1, int maxTextureSize = 2048, bool pow2 = false);
    ~ScalingVariantWidget();

    void setRemoveEnabled(bool enable);

    QString name();
    float   scale();
    int     maxTextureSize();
    bool    pow2();

    void    setPow2(bool enable);
    void    setEnabledPow2(bool enable);

signals:
    void remove();
    void valueChanged(bool needRefresh);

private slots:
    void on_removePushButton_clicked();
    void on_scaleComboBox_editTextChanged(const QString &arg1);
    void on_nameLineEdit_textChanged(const QString &arg1);
    void on_maxTextureSizeComboBox_currentTextChanged(const QString &arg1);
    void on_pow2CheckBox_toggled(bool checked);

private:
    Ui::ScalingVariantWidget *ui;
};

#endif // SCALINGVARIANTWIDGET_H
