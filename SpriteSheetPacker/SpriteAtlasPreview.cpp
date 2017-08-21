#include "SpriteAtlasPreview.h"
#include "ZoomGraphicsView.h"
#include "ui_SpriteAtlasPreview.h"

#define MAX_SCALE 10.f
#define MIN_SCALE 0.1f

SpriteAtlasPreview::SpriteAtlasPreview(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SpriteAtlasPreview)
{
    ui->setupUi(this);

    _scene = new QGraphicsScene(this);
    ui->graphicsView->setScene(_scene);
    ui->graphicsView->setAcceptDrops(false);

    _outlinesGroup = NULL;

    connect(ui->graphicsView, &ZoomGraphicsView::zoomed, [=] (bool in) {
        int oldValue = ui->zoomSlider->value();
        int zoom = in ? 2 : -2;

        ui->zoomSlider->setValue(oldValue + zoom);
    });
}

SpriteAtlasPreview::~SpriteAtlasPreview()
{
    delete ui;
}

void SpriteAtlasPreview::setAtlas(const SpriteAtlas& atlas, PixelFormat pixelFormat, bool premultiplied) {

    if (_scene) {
        if (_outlinesGroup)
            _scene->destroyItemGroup(_outlinesGroup);

        _scene->clear();
    }

    QList<QGraphicsItem*> outlineItems;
    QString infoString;
    float atlasPositionX = 0;
    for (auto it = atlas.outputData().begin(); it != atlas.outputData().end(); ++it) {
        auto outputData = (*it);
        auto atlasImage = outputData._atlasImage;
        auto spriteFrames = outputData._spriteFrames;
        if (atlasImage.isNull()) continue;
        if (!spriteFrames.size()) continue;


        QGraphicsPixmapItem* atlasPixmapItem = _scene->addPixmap(QPixmap::fromImage(convertImage(atlasImage, pixelFormat, premultiplied)));
        atlasPixmapItem->setPos(atlasPositionX, 0);
        atlasPositionX += atlasPixmapItem->boundingRect().width() + 100;

        auto rect = _scene->addRect(atlasPixmapItem->sceneBoundingRect(), QPen(Qt::darkRed), QBrush(QPixmap("://res/patterns_transparent.png")));
        rect->setZValue(-1);

        QColor brushColor(Qt::blue);
        brushColor.setAlpha(100);
        QColor polygonColor(Qt::darkGreen);
        polygonColor.setAlpha(100);
        QColor convexColor(Qt::yellow);
        convexColor.setAlpha(100);

        for(auto it = spriteFrames.begin(); it != spriteFrames.end(); ++it) {
            bool skip = false;
            for (auto identicalFrame: atlas.identicalFrames()) {
                if (skip) break;
                for (auto frame: identicalFrame) {
                    if (frame == it.key()) {
                        skip = true;
                        break;
                    }
                }
            }
            if (skip) continue;

            auto spriteFrame = it.value();
            QPoint delta = spriteFrame.frame.topLeft();

            if (atlas.algorithm() == "Rect") {
                auto rectItem = _scene->addRect(spriteFrame.frame, QPen(Qt::white), QBrush(brushColor));
                rectItem->setPos(atlasPixmapItem->pos());
                rectItem->setToolTip(it.key());
                outlineItems.push_back(rectItem);
            }

            if (spriteFrame.triangles.indices.size()) {
                for (int i=0; i<spriteFrame.triangles.indices.size(); i+=3) {
                    QPointF v1 = spriteFrame.triangles.verts[spriteFrame.triangles.indices[i+0]] + delta;
                    QPointF v2 = spriteFrame.triangles.verts[spriteFrame.triangles.indices[i+1]] + delta;
                    QPointF v3 = spriteFrame.triangles.verts[spriteFrame.triangles.indices[i+2]] + delta;

                    auto triangleItem = _scene->addPolygon(QPolygonF() << v1 << v2 << v3, QPen(Qt::white), QBrush(polygonColor));
                    triangleItem->setPos(atlasPixmapItem->pos());
                    triangleItem->setToolTip(QString("%1\nTriangles: %2").arg(it.key()).arg(spriteFrame.triangles.indices.size() / 3));
                    outlineItems.push_back(triangleItem);
                }
            }

    //        QPolygon polygon;
    //        for (auto point: spriteFrame.triangles.debugPoints) {
    //            polygon << QPoint(point.x(), point.y());
    //        }
    //        outlineItems.push_back(_scene->addPolygon(polygon, QPen(Qt::red), QBrush(convexColor)));

    //        for (auto point: spriteFrame.triangles.debugPoints) {
    //            auto rectItem = _scene->addRect(QRectF(point.x() + delta.x(), point.y() + delta.y(), 1, 1), QPen(Qt::red), QBrush(Qt::red));
    //            outlineItems.push_back(rectItem);
    //        }

            // show identical statistics
            auto identicalFrames = atlas.identicalFrames().find(it.key());
            if (identicalFrames != atlas.identicalFrames().end()) {
                auto identicalItem = _scene->addPixmap(QPixmap(":/res/icon-identical.png"));
                QString identicalString;
                identicalString += it.key() + "\n";
                for (auto frame: identicalFrames.value()) {
                    identicalString += frame + "\n";
                }
                identicalItem->setToolTip(identicalString);
                identicalItem->setPos(spriteFrame.frame.topLeft());
            }
        }

        float ram = (atlasImage.width() * atlasImage.height() * 4) / 1024.f / 1024.f;
        if (!infoString.isEmpty())
            infoString += "\n";
        infoString += QString("%1x%2x%3 (RAM: %4MB)").arg(atlasImage.width()).arg(atlasImage.height()).arg(4).arg(ram, 0, 'f', 2);
    }

    ui->labelAtlasInfo->setText(infoString);

    _outlinesGroup = _scene->createItemGroup(outlineItems);
    _outlinesGroup->setVisible(ui->displayOutlinesCheckBox->isChecked());

    _scene->setSceneRect(_scene->itemsBoundingRect());
    ui->graphicsView->update();
}

void SpriteAtlasPreview::on_toolButtonZoomOut_clicked() {
    ui->zoomSlider->setValue(ui->zoomSlider->value() - ui->zoomSlider->singleStep());
}

void SpriteAtlasPreview::on_toolButtonZoomIn_clicked() {
    ui->zoomSlider->setValue(ui->zoomSlider->value() + ui->zoomSlider->singleStep());
}

void SpriteAtlasPreview::on_toolButtonZoom1x1_clicked() {
    ui->zoomSlider->setValue(0);
}

void SpriteAtlasPreview::on_toolButtonZoomFit_clicked() {
    QRectF rect = _scene->sceneRect();
    rect.setLeft(rect.left() - rect.width() * 0.05f);
    rect.setRight(rect.right() + rect.width() * 0.05f);
    rect.setTop(rect.top() - rect.height() * 0.05f);
    rect.setBottom(rect.bottom() + rect.height() * 0.05f);

    ui->graphicsView->fitInView(rect, Qt::KeepAspectRatio);
    QTransform tr = ui->graphicsView->transform();
    float scale = tr.m11() - 1;
    int value = 0;
    if (scale > 0) {
        value = (scale / (MAX_SCALE - 1.f)) * ui->zoomSlider->maximum();
    } else {
        value = (scale / (MIN_SCALE - 1.f)) * ui->zoomSlider->minimum();
    }
    if (value > 0) {
        value = ((int)(value/(float)ui->zoomSlider->singleStep()))*ui->zoomSlider->singleStep();
    } else {
        value = (floor(value/(float)ui->zoomSlider->singleStep()))*ui->zoomSlider->singleStep();
    }

    ui->zoomSlider->setValue(value);
    on_zoomSlider_valueChanged(value);
}

void SpriteAtlasPreview::on_zoomSlider_valueChanged(int value) {
    float scale = 1;
    if (value < 0) {
        float t = (float)value / ui->zoomSlider->minimum();
        scale = 1.f + (MIN_SCALE - 1.f) * t;
    } else if (value > 0) {
        float t = (float)value / ui->zoomSlider->maximum();
        scale = 1.f + (MAX_SCALE - 1.f) * t;
    }
    ui->graphicsView->setTransform(QTransform::fromScale(scale, scale));

    ui->labelZoomPercent->setText(QString::number((int)(scale * 100)) + " %");
}

void SpriteAtlasPreview::on_displayOutlinesCheckBox_clicked(bool checked) {
    QSettings settings;
    settings.setValue("MainWindow/displayOutlines", checked);

    if (_outlinesGroup)
        _outlinesGroup->setVisible(checked);
}
