#include <QtXml>

#include "MainWindow.h"
#include "PublishSpriteSheet.h"
#include "ScalingVariantWidget.h"
#include "SpritePackerProjectFile.h"
#include "AboutDialog.h"
#include "PreferencesDialog.h"
#include "PublishStatusDialog.h"
#include "ui_MainWindow.h"

#include "PListParser.h"
#include "PListSerializer.h"

#define MAX_RECENT 10
#define MAX_SCALE 10.f
#define MIN_SCALE 0.1f

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

#if defined(Q_OS_WIN)
    setWindowIcon(QIcon("SpritePacker.ico"));
#elif defined(Q_OS_MAC)
    setWindowIcon(QIcon("SpritePacker.icns"));
#endif

    setUnifiedTitleAndToolBarOnMac(true);

    _scene = new QGraphicsScene(this);
    _scene->setBackgroundBrush(QBrush(Qt::darkGray));
    ui->graphicsView->setScene(_scene);
    ui->graphicsView->setAcceptDrops(false);
    _outlinesGroup = NULL;

    _spritesTreeWidget = new SpritesTreeWidget(ui->spritesDockWidgetContents);
    connect(_spritesTreeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(spritesTreeWidgetItemSelectionChanged()));
    ui->spritesDockWidgetLayout->addWidget(_spritesTreeWidget);

    // configure default values
    ui->trimSpinBox->setValue(1);
    ui->textureBorderSpinBox->setValue(0);
    ui->spriteBorderSpinBox->setValue(2);

    connect(ui->trimSpinBox, SIGNAL(valueChanged(int)), this, SLOT(propertiesValueChanged(int)));
    connect(ui->textureBorderSpinBox, SIGNAL(valueChanged(int)), this, SLOT(propertiesValueChanged(int)));
    connect(ui->spriteBorderSpinBox, SIGNAL(valueChanged(int)), this, SLOT(propertiesValueChanged(int)));
    connect(ui->trimSpinBox, SIGNAL(valueChanged(int)), this, SLOT(propertiesValueChanged(int)));
    connect(ui->epsilonHorizontalSlider, SIGNAL(sliderMoved(int)), this, SLOT(propertiesValueChanged(int)));
    connect(ui->pow2ComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(propertiesValueChanged(int)));
    connect(ui->maxTextureSizeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(propertiesValueChanged(int)));
    connect(ui->algorithmComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(propertiesValueChanged(int)));
    connect(ui->trimModeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(propertiesValueChanged(int)));

    on_addScalingVariantPushButton_clicked();

    // setup open button
    _openButton = new QToolButton(this);
    _openButton->setIcon(ui->actionOpen->icon());
    _openButton->setText(ui->actionOpen->text());
    _openButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    connect(_openButton, SIGNAL(pressed()), this, SLOT(on_actionOpen_triggered()));
    ui->mainToolBar->insertWidget(ui->actionSave, _openButton);

    // layout preferences action on right side in toolbar
    QWidget* empty = new QWidget();
    empty->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
    ui->mainToolBar->insertWidget(ui->actionPreferences ,empty);

    createRefreshButton();
    setAcceptDrops(true);

    refreshFormats();
    refreshOpenRecentMenu();

    QSettings settings;
    restoreGeometry(settings.value("MainWindow/geometry").toByteArray());
    restoreState(settings.value("MainWindow/state").toByteArray());
}

MainWindow::~MainWindow() {
    QSettings settings;
    settings.setValue("MainWindow/geometry", saveGeometry());
    settings.setValue("MainWindow/state", saveState());
    delete ui;
}

void MainWindow::refreshFormats() {
    QSettings settings;
    QStringList formatsFolder;
    formatsFolder.push_back(QCoreApplication::applicationDirPath() + "/defaultFormats");
    formatsFolder.push_back(settings.value("Preferences/customFormatFolder").toString());

    // load formats
    PublishSpriteSheet::formats().clear();
    for (auto folder: formatsFolder) {
        if (QDir(folder).exists()) {
            QDirIterator fileNames(folder, QStringList() << "*.js", QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot);
            while(fileNames.hasNext()) {
                fileNames.next();
                PublishSpriteSheet::addFormat(fileNames.fileInfo().baseName(), fileNames.filePath());
            }
        }
    }

    QString prevFormat = ui->dataFormatComboBox->currentText();
    ui->dataFormatComboBox->clear();
    for (auto format: PublishSpriteSheet::formats().keys()) {
        QString fileName(PublishSpriteSheet::formats()[format]);
        ui->dataFormatComboBox->addItem(QIcon(fileName.left(fileName.lastIndexOf(".")) + ".png"), format);
    }
    ui->dataFormatComboBox->setCurrentText(prevFormat);
}

void MainWindow::refreshOpenRecentMenu() {
    QSettings settings;
    QStringList openRecentList = settings.value("openRecentList").toStringList();
    ui->menuOpen_recent->clear();
    for (auto file: openRecentList) {
        QAction* recentAction = ui->menuOpen_recent->addAction(file);
        connect(recentAction, SIGNAL(triggered()), this, SLOT(openRecent()));
        if (!QFileInfo(file).exists()) {
            recentAction->setEnabled(false);
        }
    }

    if (!openRecentList.empty()) {
        _openButton->setPopupMode(QToolButton::MenuButtonPopup);
        _openButton->setMenu(ui->menuOpen_recent);
    } else {
        _openButton->setMenu(NULL);
    }
}

void MainWindow::openRecent() {
    QAction* senderAction = dynamic_cast<QAction*>(sender());
    openSpritePackerProject(senderAction->text());
}

void MainWindow::createRefreshButton() {
    QSettings settings;
    //setup enable/disable auto refresh atlas checkbox
    auto refreshFrame = new QFrame(this);
    auto vLayout = new QVBoxLayout(refreshFrame);
    // button
    auto refreshButton = new QToolButton(this);
    refreshButton->setAutoRaise(true);
    refreshButton->setIconSize(QSize(24, 24));
    refreshButton->setIcon(QIcon(":/res/refresh-32.png"));
    connect(refreshButton, &QToolButton::pressed, [this](){
        refreshAtlas();
    });
    // checkbox
    auto autoRefreshCheckbox = new QCheckBox("Auto refresh", this);
    auto font = autoRefreshCheckbox->font();
    font.setPointSize(8);
    autoRefreshCheckbox->setFont(font);
    autoRefreshCheckbox->setChecked(settings.value("Preferences/automaticPreview", true).toBool());
    connect(autoRefreshCheckbox, &QCheckBox::toggled, [](bool value) {
        QSettings settings;
        settings.setValue("Preferences/automaticPreview", value);
    });
    // layout
    vLayout->addWidget(refreshButton, 0, Qt::AlignHCenter);
    vLayout->addWidget(autoRefreshCheckbox, 0, Qt::AlignHCenter);
    ui->mainToolBar->insertWidget(ui->actionPublish, refreshFrame);
}

void MainWindow::refreshAtlas(SpriteAtlas* atlas) {
    bool deleteAtlas = false;
    if (!atlas) {
        float scale = 1;
        if (ui->scalingVariantsGroupBox->layout()->count()) {
            ScalingVariantWidget* scalingVariantWidget = qobject_cast<ScalingVariantWidget*>(ui->scalingVariantsGroupBox->layout()->itemAt(0)->widget());
            if (scalingVariantWidget) {
                scale = scalingVariantWidget->scale();
            }
        }

        atlas = new SpriteAtlas(_spritesTreeWidget->contentList(),
                                ui->textureBorderSpinBox->value(),
                                ui->spriteBorderSpinBox->value(),
                                ui->trimSpinBox->value(),
                                ui->pow2ComboBox->currentIndex()? true:false,
                                ui->maxTextureSizeComboBox->currentText().toInt(),
                                scale);
        if (!atlas->generate()) {
            QMessageBox::critical(this, "Generate error", "Max texture size limit is small!");
            delete atlas;
            return;
        }
        deleteAtlas = true;
    }

    const QImage& atlasImage = atlas->image();

    if (_outlinesGroup)
        _scene->destroyItemGroup(_outlinesGroup);
    _scene->clear();
    _scene->addRect(atlasImage.rect(), QPen(Qt::darkRed), QBrush(QPixmap("://res/background_tran.png")));

    QGraphicsPixmapItem* atlasPixmapItem = _scene->addPixmap(QPixmap::fromImage(atlasImage));

    QList<QGraphicsItem*> outlineItems;
    QColor brushColor(Qt::blue);
    brushColor.setAlpha(100);
    for(auto spriteFrame: atlas->spriteFrames()) {
        QPoint delta = spriteFrame.mFrame.topLeft();

        outlineItems.push_back(_scene->addRect(spriteFrame.mFrame, QPen(Qt::white), QBrush(brushColor)));

        for (int i=0; i<spriteFrame.triangles.indices.size(); i+=3) {
            QPointF v1 = spriteFrame.triangles.verts[spriteFrame.triangles.indices[i+0]].v + delta;
            QPointF v2 = spriteFrame.triangles.verts[spriteFrame.triangles.indices[i+1]].v + delta;
            QPointF v3 = spriteFrame.triangles.verts[spriteFrame.triangles.indices[i+2]].v + delta;

            auto polygonItem = _scene->addPolygon(QPolygonF() << v1 << v2 << v3);
            polygonItem->setPen(QPen(Qt::white));
            polygonItem->setBrush(QBrush(brushColor));
            outlineItems.push_back(polygonItem);
        }
        for (auto point: spriteFrame.triangles.debugPoints) {
            auto rectItem = _scene->addRect(QRectF(point.x() + delta.x(), point.y() + delta.y(), 1, 1));
            rectItem->setPen(QPen(Qt::red));
            rectItem->setBrush(QBrush(Qt::red));
            outlineItems.push_back(rectItem);
        }
    }
    outlineItems.push_back(_scene->addRect(atlasPixmapItem->boundingRect(), QPen(Qt::darkRed)));

    _outlinesGroup = _scene->createItemGroup(outlineItems);
    _outlinesGroup->setVisible(ui->displayOutlinesCheckBox->isChecked());

    _scene->setSceneRect(atlasPixmapItem->boundingRect());

    //1024x1024x4 (RAM: 4.00MB)
    float ram = (atlasImage.width() * atlasImage.height() * 4) / 1024.f / 1024.f;
    ui->labelAtlasInfo->setText(QString("%1x%2x%3 (RAM: %4MB)").arg(atlasImage.width()).arg(atlasImage.height()).arg(4).arg(ram, 0, 'f', 2));

    if (deleteAtlas) {
        delete atlas;
    }
}

void MainWindow::openSpritePackerProject(const QString& fileName) {
    std::string suffix = QFileInfo(fileName).suffix().toStdString();
    SpritePackerProjectFile* projectFile = SpritePackerProjectFile::factory().get(suffix)();
    if (projectFile) {
        if (!projectFile->read(fileName)) {
            QMessageBox::critical(this, "ERROR", "File format error.");
            return;
        }
    } else {
        QMessageBox::critical(this, "ERROR", "Unknown project file format!");

        return;
    }

    ui->trimSpinBox->setValue(projectFile->trimThreshold());
    ui->textureBorderSpinBox->setValue(projectFile->textureBorder());
    ui->spriteBorderSpinBox->setValue(projectFile->spriteBorder());
    ui->maxTextureSizeComboBox->setCurrentText(QString::number(projectFile->maxTextureSize()));
    ui->pow2ComboBox->setCurrentIndex(projectFile->pow2()? 1:0);
    ui->dataFormatComboBox->setCurrentText(projectFile->dataFormat());
    ui->destPathLineEdit->setText(projectFile->destPath());
    ui->spriteSheetLineEdit->setText(projectFile->spriteSheetName());
    ui->optLevelSlider->setValue(projectFile->optLevel());

    while(ui->scalingVariantsGroupBox->layout()->count() > 0){
        QLayoutItem *item = ui->scalingVariantsGroupBox->layout()->takeAt(0);
        delete item->widget();
        delete item;
    }
    for (auto scalingVariant: projectFile->scalingVariants()) {
        ScalingVariantWidget* scalingVariantWidget = new ScalingVariantWidget(this, scalingVariant.name, scalingVariant.scale);
        connect(scalingVariantWidget, SIGNAL(remove()), this, SLOT(removeScalingVariant()));
        ui->scalingVariantsGroupBox->layout()->addWidget(scalingVariantWidget);

        if (ui->scalingVariantsGroupBox->layout()->count() == 1) {
            scalingVariantWidget->setRemoveEnabled(false);
        }
    }

    _spritesTreeWidget->clear();
    _spritesTreeWidget->addContent(projectFile->srcList());
    refreshAtlas();
    // fit scene before open project
    on_toolButtonZoomFit_clicked();


    _currentProjectFileName = fileName;
    setWindowTitle(_currentProjectFileName + " - SpriteSheet Packer");

    // add to recent file
    QSettings settings;
    QStringList openRecentList = settings.value("openRecentList").toStringList();
    openRecentList.removeAll(fileName);
    openRecentList.prepend(fileName);
    while (openRecentList.size() > MAX_RECENT)
        openRecentList.removeLast();

    settings.setValue("openRecentList", openRecentList);
    settings.sync();
    refreshOpenRecentMenu();
}

void MainWindow::saveSpritePackerProject(const QString& fileName) {
    std::string suffix = QFileInfo(fileName).suffix().toStdString();
    SpritePackerProjectFile* projectFile = SpritePackerProjectFile::factory().get(suffix)();
    if (!projectFile) {
        QMessageBox::critical(this, "ERROR", "Unknown project file format!");
        return;
    }
    projectFile->setTrimThreshold(ui->trimSpinBox->value());
    projectFile->setTextureBorder(ui->textureBorderSpinBox->value());
    projectFile->setSpriteBorder(ui->spriteBorderSpinBox->value());
    projectFile->setMaxTextureSize(ui->maxTextureSizeComboBox->currentText().toInt());
    projectFile->setPow2(ui->pow2ComboBox->currentIndex()? true:false);
    projectFile->setDataFormat(ui->dataFormatComboBox->currentText());
    projectFile->setDestPath(ui->destPathLineEdit->text());
    projectFile->setSpriteSheetName(ui->spriteSheetLineEdit->text());
    projectFile->setOptLevel(ui->optLevelSlider->value());

    QVector<ScalingVariant> scalingVariants;
    for (int i=0; i<ui->scalingVariantsGroupBox->layout()->count(); ++i) {
        ScalingVariantWidget* scalingVariantWidget = qobject_cast<ScalingVariantWidget*>(ui->scalingVariantsGroupBox->layout()->itemAt(i)->widget());
        if (scalingVariantWidget) {
            ScalingVariant scalingVariant;
            scalingVariant.name = scalingVariantWidget->name();
            scalingVariant.scale = scalingVariantWidget->scale();
            scalingVariants.push_back(scalingVariant);
        }
    }
    projectFile->setScalingVariants(scalingVariants);
    projectFile->setSrcList(_spritesTreeWidget->contentList());

    if (!projectFile->write(fileName)) {
        QMessageBox::critical(this, "ERROR", QString("Not support write to [%1] format.").arg(suffix.c_str()));
        return;
    }

    _currentProjectFileName = fileName;
    setWindowTitle(_currentProjectFileName + " - SpriteSheet Packer");

    // add to recent file
    QSettings settings;
    QStringList openRecentList = settings.value("openRecentList").toStringList();
    openRecentList.removeAll(fileName);
    openRecentList.prepend(fileName);
    while (openRecentList.size() > MAX_RECENT)
        openRecentList.removeLast();

    settings.setValue("openRecentList", openRecentList);
    settings.sync();
    refreshOpenRecentMenu();
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()) {
        if (event->mimeData()->hasUrls()) {
            event->acceptProposedAction();
            return;
        }
    }
    event->ignore();
}

void MainWindow::dropEvent(QDropEvent* event) {
    QStringList filesList;
    for (auto url: event->mimeData()->urls()) {
        QFileInfo fi(url.toLocalFile());
        filesList.push_back(fi.canonicalFilePath());
    }
    if (filesList.size()) {
        _spritesTreeWidget->addContent(filesList);
        refreshAtlas();
    }
    event->accept();
}

void MainWindow::on_actionNew_triggered() {
    ui->spriteSheetLineEdit->setText("");
    _spritesTreeWidget->clear();

    _currentProjectFileName.clear();
    setWindowTitle("SpriteSheet Packer");

    refreshAtlas();
}

void MainWindow::on_actionOpen_triggered() {
    QSettings settings;
    QString dir;
    if (!_currentProjectFileName.isEmpty()) {
        dir = _currentProjectFileName;
    } else {
        dir = settings.value("spritePackerFileName", QDir::currentPath()).toString();
    }

    QString selectedFilter;
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open project file."),
                                                    dir,
                                                    tr("Supported formats (*.json *.sp *.tps)"));
    qDebug() << selectedFilter;
    if (!fileName.isEmpty()) {
        settings.setValue("spritePackerFileName", fileName);
        openSpritePackerProject(fileName);
    }
}

void MainWindow::on_actionSave_triggered() {
    if (_currentProjectFileName.isEmpty()) {
        on_actionSaveAs_triggered();
    } else {
        saveSpritePackerProject(_currentProjectFileName);
    }
}

void MainWindow::on_actionSaveAs_triggered() {
    QSettings settings;
    QString dir;
    if (!_currentProjectFileName.isEmpty()) {
        dir = _currentProjectFileName;
    } else {
        dir = settings.value("spritePackerFileName", QDir::currentPath()).toString();
    }
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save sprite packer project file."),
                                                    dir,
                                                    tr("Sprite sheet packer (*.json)"));
    if (!fileName.isEmpty()) {
        settings.setValue("spritePackerFileName", fileName);
        saveSpritePackerProject(fileName);
    }
}

void MainWindow::on_actionAddSprites_triggered() {
    QSettings settings;
    QStringList fileNames = QFileDialog::getOpenFileNames(this,
                                                          tr("Open Image"),
                                                          settings.value("spritesPath", QDir::currentPath()).toString(),
                                                          tr("Images (*.bmp *.jpg *.jpeg *.gif *.png)"));
    if (fileNames.size()) {
        settings.setValue("spritesPath", fileNames.back());

        _spritesTreeWidget->addContent(fileNames);
        refreshAtlas();
    }
}

void MainWindow::on_actionAddFolder_triggered() {
    QSettings settings;
    QString pathName = QFileDialog::getExistingDirectory(this,
                                                         tr("Open Folder"),
                                                         settings.value("spritesPath", QDir::currentPath()).toString(),
                                                         QFileDialog::DontResolveSymlinks);
    if (!pathName.isEmpty()) {
        settings.setValue("spritesPath", pathName);
        _spritesTreeWidget->addContent(QStringList() << pathName);
        refreshAtlas();
    }
}

void MainWindow::on_actionRemove_triggered() {
    foreach (QTreeWidgetItem* item, _spritesTreeWidget->selectedItems()) {
        if (!item->parent()) {
            _spritesTreeWidget->removeItemWidget(item, 0);
            delete item;
        }
    }
    refreshAtlas();
}

void MainWindow::on_actionPublish_triggered() {
    QDir dir(ui->destPathLineEdit->text());
    if ((!dir.exists())||(ui->destPathLineEdit->text().isEmpty())) {
        QMessageBox::critical(this, "Publish error", "Destination folder invalid!");
        ui->destPathLineEdit->setFocus();
        return;
    }

    QString spriteSheetName = ui->spriteSheetLineEdit->text();
    if (spriteSheetName.isEmpty()) {
        QMessageBox::critical(this, "Publish error", "Sprite sheet name invalid!");
        ui->spriteSheetLineEdit->setFocus();
        return;
    }

    PublishStatusDialog* publishStatusDialog = new PublishStatusDialog(this);
    publishStatusDialog->setAttribute(Qt::WA_DeleteOnClose);
    publishStatusDialog->open();

    publishStatusDialog->log(QString("Publish to: " + dir.canonicalPath()), Qt::blue);
    for (int i=0; i<ui->scalingVariantsGroupBox->layout()->count(); ++i) {
        ScalingVariantWidget* scalingVariantWidget = qobject_cast<ScalingVariantWidget*>(ui->scalingVariantsGroupBox->layout()->itemAt(i)->widget());
        if (scalingVariantWidget) {
            QString variantName = scalingVariantWidget->name();
            float scale = scalingVariantWidget->scale();

            QString spriteSheetName = ui->spriteSheetLineEdit->text();
            if (spriteSheetName.contains("{v}")) {
                spriteSheetName.replace("{v}", variantName);
            } else {
                spriteSheetName = variantName + spriteSheetName;
            }
            while (spriteSheetName.at(0) == '/') {
                spriteSheetName.remove(0,1);
            }

            QFileInfo destFileInfo;
            destFileInfo.setFile(dir, spriteSheetName);
            if (dir.absolutePath() != destFileInfo.dir().absolutePath()) {
                if (!dir.mkpath(destFileInfo.dir().absolutePath())) {
                    publishStatusDialog->log("Imposible create path:" + destFileInfo.dir().absolutePath(), Qt::red);
                    continue;
                }
            }

            publishStatusDialog->log(QString("Begin publish scale variant (%1) scale: %2.").arg(spriteSheetName).arg(scale));

            SpriteAtlas atlas(_spritesTreeWidget->contentList(),
                              ui->textureBorderSpinBox->value(),
                              ui->spriteBorderSpinBox->value(),
                              ui->trimSpinBox->value(),
                              ui->pow2ComboBox->currentIndex()? true:false,
                              ui->maxTextureSizeComboBox->currentText().toInt(),
                              scale);
            if (!atlas.generate()) {
                QMessageBox::critical(this, "Generate error", "Max texture size limit is small!");
                publishStatusDialog->log("Generate error: Max texture size limit is small!", Qt::red);
                continue;
            } else if (i==0) {
                refreshAtlas(&atlas);
            }

            if (!PublishSpriteSheet::publish(destFileInfo.filePath(), ui->dataFormatComboBox->currentText(), ui->optLevelSlider->value(), atlas)) {
                publishStatusDialog->log("Publish scale variant error! See all logs for details.", Qt::red);
                continue;
            } else {
                publishStatusDialog->log("Publish scale variant complete.");
            }
        }
    }
    publishStatusDialog->log(QString("Publishing is finished."), Qt::blue);
    publishStatusDialog->complete();
}

void MainWindow::on_actionAbout_triggered() {
    AboutDialog* aboutDialog = new AboutDialog(this);
    aboutDialog->setAttribute(Qt::WA_DeleteOnClose);
    aboutDialog->exec();
}

void MainWindow::on_actionPreferences_triggered() {
    PreferencesDialog* preferencesDialog = new PreferencesDialog(this);
    preferencesDialog->setAttribute(Qt::WA_DeleteOnClose);
    if (preferencesDialog->exec()) {
        refreshFormats();
    }
}

void MainWindow::on_toolButtonZoomOut_clicked() {
    ui->zoomSlider->setValue(ui->zoomSlider->value() - ui->zoomSlider->singleStep());
}

void MainWindow::on_toolButtonZoomIn_clicked() {
    ui->zoomSlider->setValue(ui->zoomSlider->value() + ui->zoomSlider->singleStep());
}

void MainWindow::on_toolButtonZoom1x1_clicked() {
    ui->zoomSlider->setValue(0);
}

void MainWindow::on_toolButtonZoomFit_clicked() {
    ui->graphicsView->fitInView(_scene->sceneRect(), Qt::KeepAspectRatio);
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

void MainWindow::on_zoomSlider_valueChanged(int value) {
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

void MainWindow::on_optLevelSlider_valueChanged(int value) {
    ui->optLevelText->setText(QString::number(value));
}

void MainWindow::spritesTreeWidgetItemSelectionChanged() {
    ui->actionRemove->setEnabled(false);
    foreach (QTreeWidgetItem* item, _spritesTreeWidget->selectedItems()) {
        if (!item->parent()) {
            ui->actionRemove->setEnabled(true);
            return;
        }
    }
}

void MainWindow::on_maxTextureSizeComboBox_currentTextChanged(const QString &) {
    bool isValideSaze;
    if (ui->maxTextureSizeComboBox->currentText().toInt(&isValideSaze) > 8192) {
        isValideSaze = false;
    }

    QPalette comboboxPalette = ui->maxTextureSizeComboBox->palette();
    if (!isValideSaze) {
        comboboxPalette.setColor(QPalette::Text, Qt::red);
    } else {
        comboboxPalette.setColor(QPalette::Text, Qt::black);
    }
    ui->maxTextureSizeComboBox->setPalette(comboboxPalette);
}

void MainWindow::on_destFolderToolButton_clicked() {
    QString destPath = ui->destPathLineEdit->text();
    destPath = QFileDialog::getExistingDirectory(this,
                                                 tr("Destination folder"),
                                                 destPath,
                                                 QFileDialog::DontResolveSymlinks);
    if (!destPath.isEmpty()) {
        ui->destPathLineEdit->setText(QDir(destPath).canonicalPath());
    }
}

void MainWindow::on_dataFormatSetupToolButton_clicked() {
    on_actionPreferences_triggered();
}


void MainWindow::on_addScalingVariantPushButton_clicked() {
    std::vector<std::pair<std::string, float>> defaultScaling = {
        std::make_pair("hdr/", 1),
        std::make_pair("hd/", 0.5f),
        std::make_pair("sd/", 0.25f),
    };

    int index = ui->scalingVariantsGroupBox->layout()->count();
    if (index >= defaultScaling.size()) {
        QMessageBox::warning(this, "Scaling variants", "Scaling variants is limited by 3.");
        return;
    }

    ScalingVariantWidget* scalingVariantWidget = new ScalingVariantWidget(this, defaultScaling[index].first.c_str(), defaultScaling[index].second);
    connect(scalingVariantWidget, SIGNAL(remove()), this, SLOT(removeScalingVariant()));
    ui->scalingVariantsGroupBox->layout()->addWidget(scalingVariantWidget);

    if (ui->scalingVariantsGroupBox->layout()->count() == 1) {
        ScalingVariantWidget* scalingVariantWidget = qobject_cast<ScalingVariantWidget*>(ui->scalingVariantsGroupBox->layout()->itemAt(0)->widget());
        if (scalingVariantWidget) {
           scalingVariantWidget->setRemoveEnabled(false);
        }
    }
}

void MainWindow::removeScalingVariant() {
    qDebug() << "remove" << sender();
    ScalingVariantWidget* scalingVariantWidget = qobject_cast<ScalingVariantWidget*>(sender());
    if (scalingVariantWidget) {
        ui->scalingVariantsGroupBox->layout()->removeWidget(scalingVariantWidget);
        delete scalingVariantWidget;
    }
//    QAction* senderAction = dynamic_cast<QAction*>(sender());
}

void MainWindow::propertiesValueChanged(int val) {
    QSettings settings;

    if (settings.value("Preferences/automaticPreview", true).toBool()) {
        refreshAtlas();
    }
}

void MainWindow::on_displayOutlinesCheckBox_clicked(bool checked) {
    if (_outlinesGroup)
        _outlinesGroup->setVisible(checked);
}
