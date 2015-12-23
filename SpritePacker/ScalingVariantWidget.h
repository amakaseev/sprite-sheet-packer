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
    explicit ScalingVariantWidget(QWidget *parent = 0, const QString& name = "", float scale = 1);
    ~ScalingVariantWidget();

    void setRemoveEnabled(bool enable);
    bool isValideScale();

    QString name();
    float   scale();

signals:
    void remove();

private slots:
    void on_removePushButton_clicked();
    void on_scaleComboBox_editTextChanged(const QString &arg1);

private:
    Ui::ScalingVariantWidget *ui;
};

#endif // SCALINGVARIANTWIDGET_H
