#include "StatusBarWidget.h"
#include "ui_StatusBarWidgetatusbarwidget.h"

StatusBarWidget::StatusBarWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::StatusBarWidget)
{
    ui->setupUi(this);

    ui->iconLabel->setVisible(false);
    ui->messageLabel->setVisible(false);
    ui->spinnerLabel->setVisible(false);
}

StatusBarWidget::~StatusBarWidget() {
    delete ui;
}

void StatusBarWidget::showMessage(const QString& message, const QPixmap& pixmap) {
    if (pixmap.isNull()) {
        ui->iconLabel->setVisible(false);
    } else {
        ui->iconLabel->setPixmap(pixmap);
        ui->iconLabel->setVisible(true);
    }
    ui->messageLabel->setVisible(true);
    ui->messageLabel->setText(message);
}

void StatusBarWidget::showSpinner(const QString& message) {
    if (message.isEmpty()) {
        ui->messageLabel->setVisible(false);
    } else {
        ui->messageLabel->setText(message);
        ui->messageLabel->setVisible(true);
    }
    ui->iconLabel->setVisible(false);

    if (!ui->spinnerLabel->isVisible()) {
        QMovie* spinnerMovie = new QMovie("://res/spinner.gif");
        spinnerMovie->start();
        ui->spinnerLabel->setVisible(true);
        ui->spinnerLabel->setAttribute(Qt::WA_TranslucentBackground, true);
        ui->spinnerLabel->setAttribute(Qt::WA_NoSystemBackground);
        ui->spinnerLabel->setMovie(spinnerMovie);
    }
}

void StatusBarWidget::hideSpinner() {
    ui->spinnerLabel->setVisible(false);
    ui->messageLabel->setVisible(false);
}
