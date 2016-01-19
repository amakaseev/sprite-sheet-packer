#include "PublishStatusDialog.h"
#include "ui_PublishStatusDialog.h"

QTextEdit* gTextEdit = NULL;
void messageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    QByteArray localMsg = msg.toLocal8Bit();
    switch (type) {
    case QtInfoMsg:
        fprintf(stderr, "%s\n", localMsg.constData());
        break;
    case QtDebugMsg:
        fprintf(stderr, "%s\n", localMsg.constData());
        break;
    case QtWarningMsg:
        fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        abort();
    }
    if(gTextEdit) {
        switch (type) {
        case QtInfoMsg:
        case QtDebugMsg:
        case QtWarningMsg:
        case QtCriticalMsg:
            gTextEdit->append(msg);
            break;
        case QtFatalMsg:
            abort();
        }
        QCoreApplication::processEvents();
    }
}

PublishStatusDialog::PublishStatusDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PublishStatusDialog)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);


    if (parent) {
        int width = parent->width() / 2;
        int height = parent->height() / 2;
        setGeometry(width/2, height/2, width, width/2);
    }

    ui->logsTextEdit->setTextColor(Qt::darkGreen);

    gTextEdit = ui->logsTextEdit;
    qInstallMessageHandler(messageOutput);

    QSettings settings;
    ui->tabWidget->setCurrentIndex(settings.value("PublishStatusDialog/currentTab", 0).toInt());
    ui->hideCheckBox->setChecked(settings.value("PublishStatusDialog/hideCheckBox", false).toBool());
}

PublishStatusDialog::~PublishStatusDialog() {
    QSettings settings;
    settings.setValue("PublishStatusDialog/currentTab", ui->tabWidget->currentIndex());
    settings.setValue("PublishStatusDialog/hideCheckBox", ui->hideCheckBox->isChecked());

    qInstallMessageHandler(NULL);
    gTextEdit = NULL;
    delete ui;
}

void PublishStatusDialog::reject() {
//    Escape Key
//    If the user presses the Esc key in a dialog, QDialog::reject() will be called.
//    This will cause the window to close: The close event cannot be ignored.
}

void PublishStatusDialog::log(const QString& msg, const QColor& color) {
    ui->smartTextEdit->setTextColor(color);
    ui->smartTextEdit->append(msg);

    ui->logsTextEdit->setTextColor((color != Qt::red)? (Qt::yellow):(Qt::red));
    ui->logsTextEdit->append(msg);
    ui->logsTextEdit->setTextColor(Qt::darkGreen);

    if (color == Qt::red) {
        ui->hideCheckBox->setChecked(false);
    }
    QCoreApplication::processEvents();
}


bool PublishStatusDialog::complete() {
    ui->completePushButton->setEnabled(true);
    if (ui->hideCheckBox->isChecked()) {
        accept();
        return true;
    }
    return false;
}

void PublishStatusDialog::on_completePushButton_clicked() {
    accept();
}
