
#include "AnimationPreviewDialog.h"
#include "SpritesTreeWidget.h"

#include "ui_AnimationPreviewDialog.h"

AnimationPreviewDialog* AnimationPreviewDialog::_instance = nullptr;
AnimationPreviewDialog::AnimationPreviewDialog(SpritesTreeWidget* spritesTreeWidget, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AnimationPreviewDialog)
{
    setAttribute(Qt::WA_DeleteOnClose);
    ui->setupUi(this);

    _instance = this;
    _animationTimer = -1;
    _pixmapItem = nullptr;
    _spritesTreeWidget = spritesTreeWidget;

    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);

    ui->graphicsView->setScene(&_scene);

    ui->spritesTreeView->setModel(spritesTreeWidget->model());
    ui->spritesTreeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->spritesTreeView->setIconSize(QSize(24, 24));
    ui->spritesTreeView->setRootIsDecorated(true);
    ui->spritesTreeView->header()->setVisible(false);
    //connect(ui->spritesTreeView, SIGNAL(itemSelectionChanged()), this, SLOT(spritesTreeWidgetItemSelectionChanged()));

    QSettings settings;
    restoreGeometry(settings.value("AnimationPreviewDialog/geometry").toByteArray());
    ui->framePerSecondSpinBox->setValue(settings.value("AnimationPreviewDialog/framePerSecondSpinBox", 24).toInt());
    ui->repeatToolButton->setChecked(settings.value("AnimationPreviewDialog/repeatToolButton", true).toBool());

    ui->splitter->setStretchFactor(0, 0);
    ui->splitter->setStretchFactor(1, 1);
}

AnimationPreviewDialog::~AnimationPreviewDialog() {
    QSettings settings;
    settings.setValue("AnimationPreviewDialog/geometry", saveGeometry());
    settings.setValue("AnimationPreviewDialog/framePerSecondSpinBox", ui->framePerSecondSpinBox->value());
    settings.setValue("AnimationPreviewDialog/repeatToolButton", ui->repeatToolButton->isChecked());

    _instance = nullptr;
    delete ui;
}

void AnimationPreviewDialog::setPreviewPixmap(const QPixmap &pixmap) {
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

AnimationInfo AnimationPreviewDialog::detectAnimations(const QPair<QString, QString>& item, QList< QPair<QString, QString> >& items) {
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

void AnimationPreviewDialog::timerEvent(QTimerEvent* /*event*/) {
    on_nextFrameToolButton_clicked();
}

//void AnimationPreviewDialog::on_spritesTreeView_itemSelectionChanged() {
//    _frames.clear();

//    auto items = ui->spritesTreeView->selectionModel()->selectedItems();
//    for (auto item: items) {
//        if (item->childCount()) {
//            scanFolder(item);
//        } else if (!item->data(0, Qt::UserRole).toString().isEmpty()) {
//            _frames.push_back(QPixmap(item->data(0, Qt::UserRole).toString()));
//        }
//    }

//    if (_frames.size()) {
//        setPreviewPixmap(_frames.front());
//        ui->framesSlider->setMaximum(_frames.size() - 1);
//    } else {
//        ui->framesSlider->setMaximum(0);
//    }
//}

void AnimationPreviewDialog::on_framePerSecondSpinBox_valueChanged(int value) {
    if (_animationTimer !=-1) {
        killTimer(_animationTimer);
        _animationTimer = startTimer(1000.f/value);
    }
}

void AnimationPreviewDialog::on_framesSlider_valueChanged(int value) {
    if ((value >= 0) && (value < _frames.size())) {
        setPreviewPixmap(_frames[value]);
    } else {
        setPreviewPixmap(QPixmap());
        //ui->previewLabel->setText("No sprites selected");
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

void AnimationPreviewDialog::on_autoDetectPushButton_clicked() {
    auto fileList = _spritesTreeWidget->fileList();

    ui->comboBox->clear();
    while (fileList.length()) {
        auto first = fileList.first();
        fileList.erase(fileList.begin());
        auto animation = detectAnimations(first, fileList);
        if (!animation.name.isEmpty()) {
            ui->comboBox->addItem(animation.name);
        }
    }


}
