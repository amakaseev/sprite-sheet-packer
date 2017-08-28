
#include "AnimationDialog.h"
#include "SpritesTreeWidget.h"

#include "ui_AnimationDialog.h"

AnimationDialog* AnimationDialog::_instance = nullptr;
AnimationDialog::AnimationDialog(SpritesTreeWidget* spritesTreeWidget, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AnimationDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_DeleteOnClose);

    _instance = this;
    _pixmapItem = nullptr;
    _currentFrame = 0;
    _spritesTreeWidget = spritesTreeWidget;

    _animationTimer = new ElapsedTimer(this);
    connect(_animationTimer, SIGNAL(timeout(int)), this, SLOT(updateFrame(int)));

    ui->animationPropertiesGroupBox->setEnabled(false);
    ui->graphicsView->setScene(&_scene);

    ui->removeAnimationToolButton->setEnabled(false);

    QSettings settings;
    restoreGeometry(settings.value("AnimationDialog/geometry").toByteArray());

    ui->splitter->setStretchFactor(0, 0);
    ui->splitter->setStretchFactor(1, 1);
}

AnimationDialog::~AnimationDialog() {
    QSettings settings;
    settings.setValue("AnimationDialog/geometry", saveGeometry());

    _instance = nullptr;
    delete ui;
}

void AnimationDialog::setPreviewPixmap(const QPixmap &pixmap) {
    if (!_pixmapItem) {
        _pixmapItem = _scene.addPixmap(QPixmap());
    }

    _pixmapItem->setPixmap(pixmap);
    _scene.setSceneRect(_scene.itemsBoundingRect());
}

bool machString(QString a, QString b) {
    QString baseName;
    QString number;

    a = QDir::fromNativeSeparators(QFileInfo(a).path() + QDir::separator() + QFileInfo(a).baseName());
    b = QDir::fromNativeSeparators(QFileInfo(b).path() + QDir::separator() + QFileInfo(b).baseName());

    bool equal = true;
    for (int i=0; i<b.length(); ++i) {
        if (equal && (i<a.length()) && (a[i] == b[i])) {
            baseName += a[i];
        } else {
            equal = false;
            number += b[i];
        }
    }

    bool ok;
    number.toInt(&ok, 10);
    return ok;
}

QString clearAnimatonName(const QString& name) {
    QString animationName = QFileInfo(name).baseName();
    for (int i=animationName.length()-1; i>=0; --i) {
        if (animationName[i].isDigit()) {
            animationName.remove(i, 1);
        } else {
            if ((animationName.length() > 1) && (animationName[i] == '_')) {
                animationName.remove(i, 1);
            }
            break;
        }
    }
    return animationName;
}

AnimationInfo AnimationDialog::detectAnimations(const QPair<QString, QString>& item, QList< QPair<QString, QString> >& items) {
    AnimationInfo animation;
    auto it = items.begin();
    while(it != items.end()) {
        if (machString(item.second, (*it).second)) {
            animation.frames.push_back((*it));
            it = items.erase(it);
        } else {
            ++it;
        }
    }
    if (animation.frames.length()) {
        animation.frames.push_front(item);
        animation.name = clearAnimatonName(item.second);
    }
    return animation;
}

void AnimationDialog::updateFrame(int elapsed) {
    _currentFrame += (elapsed / 1000.f) * ui->framePerSecondSpinBox->value();
    while (_currentFrame > ui->framesSlider->maximum()) {
        if (ui->repeatToolButton->isChecked()) {
            _currentFrame -= ui->framesSlider->maximum();
        } else {
            _currentFrame = ui->framesSlider->maximum();
            break;
        }
    }
    ui->framesSlider->setValue((int)_currentFrame);
}

void AnimationDialog::on_framePerSecondSpinBox_valueChanged(int value) {
    _animationTimer->setInterval(1000.f/value);

    int index = ui->animationsComboBox->currentIndex();
    if ((index>=0)&&(index<_animations.length())) {
        _animations[index].fps = value;
    }
}

void AnimationDialog::on_repeatToolButton_clicked(bool checked) {
    int index = ui->animationsComboBox->currentIndex();
    if ((index>=0)&&(index<_animations.length())) {
        _animations[index].loop = checked;
    }
}

void AnimationDialog::on_framesSlider_valueChanged(int value) {
    ui->framesListWidget->setCurrentRow(value);
}

void AnimationDialog::on_playToolButton_toggled(bool checked) {
    if (checked) {
        _animationTimer->start(1000.f/ui->framePerSecondSpinBox->value());
        ui->playToolButton->setIcon(QIcon("://res/playback/control_pause_blue.png"));
        if (ui->framesSlider->value() == ui->framesSlider->maximum()) {
            ui->framesSlider->setValue(0);
        }
        ui->framesSlider->setEnabled(false);
        ui->prevFrameToolButton->setEnabled(false);
        ui->nextFrameToolButton->setEnabled(false);
        ui->firstFrameToolButton->setEnabled(false);
        ui->lastFrameToolButton->setEnabled(false);
    } else {
        _animationTimer->stop();
        ui->playToolButton->setIcon(QIcon("://res/playback/control_play_blue.png"));
        ui->framesSlider->setEnabled(true);
        ui->prevFrameToolButton->setEnabled(true);
        ui->nextFrameToolButton->setEnabled(true);
        ui->firstFrameToolButton->setEnabled(true);
        ui->lastFrameToolButton->setEnabled(true);
    }
    _currentFrame = ui->framesSlider->value();
}

void AnimationDialog::on_prevFrameToolButton_clicked() {
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
    _currentFrame = ui->framesSlider->value();
}

void AnimationDialog::on_nextFrameToolButton_clicked() {
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
    _currentFrame = ui->framesSlider->value();
}

void AnimationDialog::on_firstFrameToolButton_clicked() {
    ui->framesSlider->setValue(ui->framesSlider->minimum());
    _currentFrame = ui->framesSlider->value();
}

void AnimationDialog::on_lastFrameToolButton_clicked() {
    ui->framesSlider->setValue(ui->framesSlider->maximum());
    _currentFrame = ui->framesSlider->value();
}

void AnimationDialog::on_autoDetectPushButton_clicked() {
    auto fileList = _spritesTreeWidget->fileList();

    if (fileList.size()) {
        auto result = QMessageBox::Yes;
        if (ui->animationsComboBox->count()) {
            result = QMessageBox::question(this,
                                           "Auto-Detect animation",
                                           "Current animations will be removed?",
                                           QMessageBox::No | QMessageBox::Yes,
                                           QMessageBox::Yes);
        }
        if (result == QMessageBox::Yes) {
            _animations.clear();
            ui->animationsComboBox->clear();
            while (fileList.length()) {
                auto first = fileList.first();
                fileList.erase(fileList.begin());
                AnimationInfo animation = detectAnimations(first, fileList);
                if (!animation.name.isEmpty()) {
                    animation.fps = 24;
                    animation.loop = true;
                    _animations.push_back(animation);
                    ui->animationsComboBox->addItem(animation.name);
                }
            }
        }
    }
}

void AnimationDialog::on_animationsComboBox_currentIndexChanged(int index) {
    ui->framesListWidget->clear();
    if ((index>=0)&&(index<_animations.length())) {
        for (auto frame: _animations[index].frames) {
            auto item = new QListWidgetItem(ui->framesListWidget);
            item->setText(QFileInfo(frame.first).baseName());
            item->setIcon(QIcon(frame.first));
            item->setData(Qt::UserRole, frame.first);
        }
        ui->framePerSecondSpinBox->setValue(_animations[index].fps);
        ui->repeatToolButton->setChecked(_animations[index].loop);
        ui->framesSlider->setMaximum(_animations[index].frames.length() - 1);
        ui->framesListWidget->setCurrentRow(0);
        ui->animationPropertiesGroupBox->setEnabled(true);
        ui->removeAnimationToolButton->setEnabled(true);
    } else {
        ui->framesSlider->setMaximum(0);
        ui->animationPropertiesGroupBox->setEnabled(false);
        ui->removeAnimationToolButton->setEnabled(false);
    }
    _currentFrame = ui->framesSlider->value();
}

void AnimationDialog::on_animationsComboBox_editTextChanged(const QString &arg1) {
    int index = ui->animationsComboBox->currentIndex();
    if ((index>=0)&&(index<_animations.length())) {
        _animations[index].name = arg1;
        ui->animationsComboBox->setItemText(index, arg1);
    }
}

void AnimationDialog::on_addAnimationToolButton_clicked() {
    int index = 0;
    if (ui->animationsComboBox->currentIndex() != -1) {
        index = ui->animationsComboBox->currentIndex() + 1;
    }
    AnimationInfo animation;
    animation.name = QString("animation_%1").arg(_animations.size());
    animation.fps = 24;
    animation.loop = true;
    _animations.insert(index, animation);
    ui->animationsComboBox->insertItem(index, animation.name);
    ui->animationsComboBox->setCurrentIndex(index);
}

void AnimationDialog::on_removeAnimationToolButton_clicked() {
    int index = ui->animationsComboBox->currentIndex();
    if (index != -1) {
        if (QMessageBox::question(this,
                                  "Remove animation",
                                  QString("Are you sure you want to delete [%1] animation?").arg(ui->animationsComboBox->currentText()),
                                  QMessageBox::No | QMessageBox::Yes,
                                  QMessageBox::Yes) == QMessageBox::Yes)
        {
            _animations.remove(index);
            ui->animationsComboBox->removeItem(index);
        }
    }
}

void AnimationDialog::on_framesListWidget_currentRowChanged(int currentRow) {
    if (currentRow >= 0) {
        setPreviewPixmap(ui->framesListWidget->item(currentRow)->data(Qt::UserRole).toString());
    } else {
        setPreviewPixmap(QPixmap());
    }
}
