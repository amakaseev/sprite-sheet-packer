
#include "AnimationPreviewDialog.h"
#include "SpritesTreeWidget.h"

#include "ui_AnimationPreviewDialog.h"

AnimationPreviewDialog* AnimationPreviewDialog::_instance = nullptr;

AnimationPreviewDialog::AnimationPreviewDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AnimationPreviewDialog)
{
    setAttribute(Qt::WA_DeleteOnClose);
    ui->setupUi(this);

    _instance = this;
    _animationTimer = -1;

    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    ui->previewLabel->setStyleSheet("background-image: url(://res/patterns_tweed.png)");

    QSettings settings;
    restoreGeometry(settings.value("AnimationPreviewDialog/geometry").toByteArray());
    ui->framePerSecondSpinBox->setValue(settings.value("AnimationPreviewDialog/framePerSecondSpinBox", 24).toInt());
    ui->repeatToolButton->setChecked(settings.value("AnimationPreviewDialog/repeatToolButton", true).toBool());
}

AnimationPreviewDialog::~AnimationPreviewDialog() {
    QSettings settings;
    settings.setValue("AnimationPreviewDialog/geometry", saveGeometry());
    settings.setValue("AnimationPreviewDialog/framePerSecondSpinBox", ui->framePerSecondSpinBox->value());
    settings.setValue("AnimationPreviewDialog/repeatToolButton", ui->repeatToolButton->isChecked());

    _instance = nullptr;
    delete ui;
}

void AnimationPreviewDialog::spritesSelectionChanged(SpritesTreeWidget* spritesTreeWidget) {
    _frames.clear();

    auto items = spritesTreeWidget->selectedItems();
    for (auto item: items) {
        if (item->childCount()) {
            scanFolder(item);
        } else if (!item->data(0, Qt::UserRole).toString().isEmpty()) {
            _frames.push_back(QPixmap(item->data(0, Qt::UserRole).toString()));
        }
    }

    if (_frames.size()) {
        ui->previewLabel->setPixmap(_frames.front());
        ui->framesSlider->setMaximum(_frames.size() - 1);
    } else {
        ui->framesSlider->setMaximum(0);
    }
}

void AnimationPreviewDialog::timerEvent(QTimerEvent* /*event*/) {
    on_nextFrameToolButton_clicked();
}

void AnimationPreviewDialog::scanFolder(QTreeWidgetItem* item) {
    for (int i=0; i<item->childCount(); ++i) {
        auto child = item->child(i);
        if (child->childCount()) {
            scanFolder(child);
        } else if (!child->data(0, Qt::UserRole).toString().isEmpty()) {
            _frames.push_back(QPixmap(child->data(0, Qt::UserRole).toString()));
        }
    }
}

void AnimationPreviewDialog::on_framePerSecondSpinBox_valueChanged(int value) {
    if (_animationTimer !=-1) {
        killTimer(_animationTimer);
        _animationTimer = startTimer(1000.f/value);
    }
}

void AnimationPreviewDialog::on_framesSlider_valueChanged(int value) {
    if ((value >= 0) && (value < _frames.size())) {
        ui->previewLabel->setPixmap(_frames[value]);
    } else {
        ui->previewLabel->setPixmap(QPixmap());
        ui->previewLabel->setText("No sprites selected");
    }
}

void AnimationPreviewDialog::on_playToolButton_toggled(bool checked) {
    if (checked) {
        _animationTimer = startTimer(1000.f/ui->framePerSecondSpinBox->value());
        ui->playToolButton->setIcon(QIcon("://res/playback/control_pause_blue.png"));
        ui->framesSlider->setValue(0);
        ui->framesSlider->setEnabled(false);
        ui->prevFrameToolButton->setEnabled(false);
        ui->nextFrameToolButton->setEnabled(false);
        ui->firstFrameToolButton->setEnabled(false);
        ui->lastFrameToolButton->setEnabled(false);
    } else {
        killTimer(_animationTimer);
        _animationTimer = -1;
        ui->playToolButton->setIcon(QIcon("://res/playback/control_play_blue.png"));
        ui->framesSlider->setEnabled(true);
        ui->prevFrameToolButton->setEnabled(true);
        ui->nextFrameToolButton->setEnabled(true);
        ui->firstFrameToolButton->setEnabled(true);
        ui->lastFrameToolButton->setEnabled(true);
    }
}

void AnimationPreviewDialog::on_prevFrameToolButton_clicked() {
    int currentFrame = ui->framesSlider->value();
    currentFrame--;
    if (currentFrame < ui->framesSlider->minimum()) {
        if (ui->repeatToolButton->isChecked()) {
            currentFrame = ui->framesSlider->maximum();
        } else {
            currentFrame = ui->framesSlider->minimum();
            ui->playToolButton->setChecked(false);
        }
    }
    ui->framesSlider->setValue(currentFrame);
}

void AnimationPreviewDialog::on_nextFrameToolButton_clicked() {
    int currentFrame = ui->framesSlider->value();
    currentFrame++;
    if (currentFrame > ui->framesSlider->maximum()) {
        if (ui->repeatToolButton->isChecked()) {
            currentFrame = ui->framesSlider->minimum();
        } else {
            currentFrame = ui->framesSlider->maximum();
            ui->playToolButton->setChecked(false);
        }
    }
    ui->framesSlider->setValue(currentFrame);
}

void AnimationPreviewDialog::on_firstFrameToolButton_clicked() {
    ui->framesSlider->setValue(ui->framesSlider->minimum());
}

void AnimationPreviewDialog::on_lastFrameToolButton_clicked() {
    ui->framesSlider->setValue(ui->framesSlider->maximum());
}
