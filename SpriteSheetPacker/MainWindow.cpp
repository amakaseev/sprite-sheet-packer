#include <QtXml>

#include "MainWindow.h"
#include "PublishSpriteSheet.h"
#include "ScalingVariantWidget.h"
#include "AboutDialog.h"
#include "PreferencesDialog.h"
#include "PublishStatusDialog.h"
#include "ui_MainWindow.h"

#include "PListParser.h"
#include "PListSerializer.h"

#define MAX_RECENT 10
#define MAX_SCALE 10.f
#define MIN_SCALE 0.1f

static inline QString toString(ImageFormat imageFormat) {
    switch (imageFormat) {
        case kPNG: return "PNG";
        case kPKM: return "PKM";
        case kPVR: return "PVR";
        case kPVR_CCZ: return "PVR_CCZ";
        default: return "PNG";
    }
}

static inline ImageFormat imageFormatFromString(const QString& imageFormat) {
    if (imageFormat == "PNG") return kPNG;
    if (imageFormat == "PKM") return kPKM;
    if (imageFormat == "PVR") return kPVR;
    if (imageFormat == "PVR_CCZ") return kPVR_CCZ;
    return kPNG;
}

static inline QString toString(PixelFormat pixelFormat) {
    switch (pixelFormat) {
        case kRGB888: return "RGB888";
        case kRGBA8888: return "RGBA8888";
        case kETC1: return "ETC1";
        case kPVRTC2: return "PVRTC2";
        case kPVRTC2A: return "PVRTC2A";
        case kPVRTC4: return "PVRTC4";
        case kPVRTC4A: return "PVRTC4A";
        default: return "RGBA8888";
    }
}

static inline PixelFormat pixelFormatFromString(const QString& pixelFormat) {
    if (pixelFormat == "RGB888") return kRGB888;
    if (pixelFormat == "RGBA8888") return kRGBA8888;
    if (pixelFormat == "ETC1") return kETC1;
    if (pixelFormat == "PVRTC2") return kPVRTC2;
    if (pixelFormat == "PVRTC2A") return kPVRTC2A;
    if (pixelFormat == "PVRTC4") return kPVRTC4;
    if (pixelFormat == "PVRTC4A") return kPVRTC4A;
    return kRGBA8888;
}

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

    _blockUISignals = false;
    _projectDirty = false;

    _spritesTreeWidget = new SpritesTreeWidget(ui->spritesDockWidgetContents);
    connect(_spritesTreeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(spritesTreeWidgetItemSelectionChanged()));
    ui->spritesDockWidgetLayout->addWidget(_spritesTreeWidget);

    // init image and pixel formats
    ui->pixelFormatComboBox->addItem(toString(kRGB888));
    ui->pixelFormatComboBox->addItem(toString(kRGBA8888));
    ui->pixelFormatComboBox->addItem(toString(kETC1));
    ui->pixelFormatComboBox->addItem(toString(kPVRTC2));
    ui->pixelFormatComboBox->addItem(toString(kPVRTC2A));
    ui->pixelFormatComboBox->addItem(toString(kPVRTC4));
    ui->pixelFormatComboBox->addItem(toString(kPVRTC4A));
    ui->pixelFormatComboBox->setCurrentIndex(kRGBA8888);
    ui->imageFormatComboBox->insertItem(kPNG, "PNG (.png)");
    ui->imageFormatComboBox->insertItem(kPKM, "PKM with ETC1 (.pkm)");
    ui->imageFormatComboBox->insertItem(kPVR, "PVR (.pvr)");
    ui->imageFormatComboBox->insertItem(kPVR_CCZ, "PVR+zlib (.pvr.ccz)");
    ui->imageFormatComboBox->setCurrentIndex(kPNG);

    // configure default values
    ui->trimSpinBox->setValue(1);
    ui->textureBorderSpinBox->setValue(0);
    ui->spriteBorderSpinBox->setValue(2);

    on_addScalingVariantPushButton_clicked();

    // setup open button
    _openButton = new QToolButton(this);
    _openButton->setIcon(ui->actionOpen->icon());
    _openButton->setText(ui->actionOpen->text());
    _openButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    connect(_openButton, SIGNAL(pressed()), this, SLOT(on_actionOpen_triggered()));
    ui->mainToolBar->insertWidget(ui->actionSave, _openButton);

    ui->optLevelSlider->setVisible(false);
    ui->optLevelText->setVisible(false);
    ui->optLevelLabel->setVisible(false);

    // layout preferences action on right side in toolbar
    QWidget* empty = new QWidget();
    empty->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    ui->mainToolBar->insertWidget(ui->actionPreferences ,empty);

    createRefreshButton();
    setAcceptDrops(true);

    refreshFormats();
    refreshOpenRecentMenu();

    // reset dirty
    _projectDirty = false;

    QSettings settings;
    
    ui->displayOutlinesCheckBox->setChecked(settings.value("MainWindow/displayOutlines").toBool());
    
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

    _blockUISignals = true;
    QString prevFormat = ui->dataFormatComboBox->currentText();
    ui->dataFormatComboBox->clear();
    for (auto format: PublishSpriteSheet::formats().keys()) {
        QString fileName(PublishSpriteSheet::formats()[format]);
        ui->dataFormatComboBox->addItem(QIcon(fileName.left(fileName.lastIndexOf(".")) + ".png"), format);
    }
    ui->dataFormatComboBox->setCurrentText(prevFormat);
    _blockUISignals = false;
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
    autoRefreshCheckbox->setChecked(settings.value("MainWindow/automaticPreview", true).toBool());
    connect(autoRefreshCheckbox, &QCheckBox::toggled, [](bool value) {
        QSettings settings;
        settings.setValue("MainWindow/automaticPreview", value);
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

        atlas->setAlgorithm(ui->algorithmComboBox->currentText());

        if (ui->trimModeComboBox->currentText() == "Polygon") {
            atlas->enablePolygonMode(true, ui->epsilonHorizontalSlider->value() / 10.f);
        }

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
    QColor polygonColor(Qt::darkGreen);
    polygonColor.setAlpha(100);
    QColor convexColor(Qt::yellow);
    convexColor.setAlpha(100);

    for(auto it = atlas->spriteFrames().begin(); it != atlas->spriteFrames().end(); ++it) {
        bool skip = false;
        for (auto identicalFrame: atlas->identicalFrames()) {
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

        if (ui->algorithmComboBox->currentText() == "Rect") {
            auto rectItem = _scene->addRect(spriteFrame.frame, QPen(Qt::white), QBrush(brushColor));
            rectItem->setToolTip(it.key());
            outlineItems.push_back(rectItem);
        }

        if (spriteFrame.triangles.indices.size()) {
            for (int i=0; i<spriteFrame.triangles.indices.size(); i+=3) {
                QPointF v1 = spriteFrame.triangles.verts[spriteFrame.triangles.indices[i+0]] + delta;
                QPointF v2 = spriteFrame.triangles.verts[spriteFrame.triangles.indices[i+1]] + delta;
                QPointF v3 = spriteFrame.triangles.verts[spriteFrame.triangles.indices[i+2]] + delta;

                auto triangleItem = _scene->addPolygon(QPolygonF() << v1 << v2 << v3, QPen(Qt::white), QBrush(polygonColor));
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
        auto identicalFrames = atlas->identicalFrames().find(it.key());
        if (identicalFrames != atlas->identicalFrames().end()) {
            auto identicalItem = _scene->addPixmap(QPixmap(":/res/identical.png"));
            QString identicalString;
            identicalString += it.key() + "\n";
            for (auto frame: identicalFrames.value()) {
                identicalString += frame + "\n";
            }
            identicalItem->setToolTip(identicalString);
            identicalItem->setPos(spriteFrame.frame.topLeft());
        }
    }

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

    _blockUISignals = true;
    ui->algorithmComboBox->setCurrentText(projectFile->algorithm());
    ui->trimModeComboBox->setCurrentText(projectFile->trimMode());
    ui->trimSpinBox->setValue(projectFile->trimThreshold());
    ui->epsilonHorizontalSlider->setValue(projectFile->epsilon() * 10);
    ui->textureBorderSpinBox->setValue(projectFile->textureBorder());
    ui->spriteBorderSpinBox->setValue(projectFile->spriteBorder());
    ui->maxTextureSizeComboBox->setCurrentText(QString::number(projectFile->maxTextureSize()));
    ui->pow2ComboBox->setCurrentIndex(projectFile->pow2()? 1:0);
    ui->dataFormatComboBox->setCurrentText(projectFile->dataFormat());
    ui->destPathLineEdit->setText(projectFile->destPath());
    ui->spriteSheetLineEdit->setText(projectFile->spriteSheetName());
    ui->imageFormatComboBox->setCurrentIndex(imageFormatFromString(projectFile->imageFormat()));
    ui->pixelFormatComboBox->setCurrentIndex(pixelFormatFromString(projectFile->pixelFormat()));
    ui->optModeComboBox->setCurrentText(projectFile->optMode());
    ui->optLevelSlider->setValue(projectFile->optLevel());

    while(ui->scalingVariantsGroupBox->layout()->count() > 0){
        QLayoutItem *item = ui->scalingVariantsGroupBox->layout()->takeAt(0);
        delete item->widget();
        delete item;
    }
    for (auto scalingVariant: projectFile->scalingVariants()) {
        ScalingVariantWidget* scalingVariantWidget = new ScalingVariantWidget(this, scalingVariant.name, scalingVariant.scale);
        connect(scalingVariantWidget, SIGNAL(remove()), this, SLOT(removeScalingVariant()));
        connect(scalingVariantWidget, SIGNAL(valueChanged()), this, SLOT(scalingVariantWidgetValueChanged()));
        ui->scalingVariantsGroupBox->layout()->addWidget(scalingVariantWidget);

        if (ui->scalingVariantsGroupBox->layout()->count() == 1) {
            scalingVariantWidget->setRemoveEnabled(false);
        }
    }

    _spritesTreeWidget->clear();
    _spritesTreeWidget->addContent(projectFile->srcList());

    _blockUISignals = false;
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

    projectFile->setAlgorithm(ui->algorithmComboBox->currentText());
    projectFile->setTrimMode(ui->trimModeComboBox->currentText());
    projectFile->setTrimThreshold(ui->trimSpinBox->value());
    projectFile->setEpsilon(ui->epsilonHorizontalSlider->value() / 10.f);
    projectFile->setTextureBorder(ui->textureBorderSpinBox->value());
    projectFile->setSpriteBorder(ui->spriteBorderSpinBox->value());
    projectFile->setMaxTextureSize(ui->maxTextureSizeComboBox->currentText().toInt());
    projectFile->setPow2(ui->pow2ComboBox->currentIndex()? true:false);
    projectFile->setDataFormat(ui->dataFormatComboBox->currentText());
    projectFile->setDestPath(ui->destPathLineEdit->text());
    projectFile->setSpriteSheetName(ui->spriteSheetLineEdit->text());
    projectFile->setImageFormat(toString((ImageFormat)ui->imageFormatComboBox->currentIndex()));
    projectFile->setPixelFormat(toString((PixelFormat)ui->pixelFormatComboBox->currentIndex()));
    projectFile->setOptMode(ui->optModeComboBox->currentText());
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

void MainWindow::setProjectDirty() {
    if (!_blockUISignals) {
        _projectDirty = true;
    }
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

    setProjectDirty();
}

void MainWindow::closeEvent(QCloseEvent *event) {
    //alreadyClosed use only for fix bug on MacOS: https://bugreports.qt.io/browse/QTBUG-43344
    static bool alreadyClosed = false;
    if (_projectDirty && !alreadyClosed) {
        QMessageBox::StandardButton resBtn = QMessageBox::question(this, "Spritesheet Packer",
                                                                    tr("Your project file has been modified.\nDo you want to save your changes?"),
                                                                    QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes,
                                                                    QMessageBox::Yes);
        if (resBtn == QMessageBox::Yes) {
            if (_currentProjectFileName.isEmpty()) {
                on_actionSaveAs_triggered();
            } else {
                saveSpritePackerProject(_currentProjectFileName);
            }
            event->accept();
            alreadyClosed = true;
        } else if (resBtn == QMessageBox::Cancel) {
            event->ignore();
        } else {
            event->accept();
            alreadyClosed = true;
        }
    }
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
    _projectDirty = false;
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

    setProjectDirty();
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

    setProjectDirty();
}

void MainWindow::on_actionRemove_triggered() {
    foreach (QTreeWidgetItem* item, _spritesTreeWidget->selectedItems()) {
        if (!item->parent()) {
            _spritesTreeWidget->removeItemWidget(item, 0);
            delete item;
        }
    }
    refreshAtlas();

    setProjectDirty();
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

    PublishSpriteSheet* publisher = new PublishSpriteSheet();
    publisher->setImageFormat((ImageFormat)ui->imageFormatComboBox->currentIndex());
    publisher->setPixelFormat((PixelFormat)ui->pixelFormatComboBox->currentIndex());

    PublishStatusDialog publishStatusDialog(this);
    publishStatusDialog.open();

    publishStatusDialog.log(QString("Publish to: " + dir.canonicalPath()), Qt::blue);
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
                    publishStatusDialog.log("Imposible create path:" + destFileInfo.dir().absolutePath(), Qt::red);
                    continue;
                }
            }

            publishStatusDialog.log(QString("Generating scale variant (%1) scale: %2.").arg(spriteSheetName).arg(scale));

            SpriteAtlas atlas(_spritesTreeWidget->contentList(),
                              ui->textureBorderSpinBox->value(),
                              ui->spriteBorderSpinBox->value(),
                              ui->trimSpinBox->value(),
                              ui->pow2ComboBox->currentIndex()? true:false,
                              ui->maxTextureSizeComboBox->currentText().toInt(),
                              scale);

            atlas.setAlgorithm(ui->algorithmComboBox->currentText());

            if (ui->trimModeComboBox->currentText() == "Polygon") {
                atlas.enablePolygonMode(true, ui->epsilonHorizontalSlider->value() / 10.f);
            }

            if (!atlas.generate()) {
                QMessageBox::critical(this, "Generate error", "Max texture size limit is small!");
                publishStatusDialog.log("Generate error: Max texture size limit is small!", Qt::red);
                continue;
            } else if (i==0) {
                refreshAtlas(&atlas);
            }

            publisher->addSpriteSheet(atlas, destFileInfo.filePath());
        }
    }

    publisher->publish(ui->dataFormatComboBox->currentText(), ui->optModeComboBox->currentText(), ui->optLevelSlider->value());

    if (ui->optModeComboBox->currentText() == "None") {
        delete publisher;
    } else {
        // TODO: you need to wait previous image optimization if they are the same
        QObject::connect(publisher, &PublishSpriteSheet::onCompleted, [this, publisher] () {
            // TODO: it would be good to show here for information about the optimization
            QMessageBox::information(this, "PNG Optimization", "PNG Optimization: complete.");
            delete publisher;
        });
    }

    publishStatusDialog.log(QString("Publishing is finished."), Qt::blue);
    if (!publishStatusDialog.complete()) {
        // This loop will wait for the window is finished
        QEventLoop loop;
        connect(&publishStatusDialog, SIGNAL(finished(int)), &loop, SLOT(quit()));
        loop.exec();
        qDebug() << "Finish publish";
    }
}

void MainWindow::on_actionAbout_triggered() {
    AboutDialog aboutDialog(this);
    aboutDialog.exec();
}

void MainWindow::on_actionPreferences_triggered() {
    PreferencesDialog preferencesDialog(this);
    if (preferencesDialog.exec()) {
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

void MainWindow::spritesTreeWidgetItemSelectionChanged() {
    ui->actionRemove->setEnabled(false);
    foreach (QTreeWidgetItem* item, _spritesTreeWidget->selectedItems()) {
        if (!item->parent()) {
            ui->actionRemove->setEnabled(true);
            return;
        }
    }
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
    connect(scalingVariantWidget, SIGNAL(valueChanged()), this, SLOT(scalingVariantWidgetValueChanged()));
    ui->scalingVariantsGroupBox->layout()->addWidget(scalingVariantWidget);

    if (ui->scalingVariantsGroupBox->layout()->count() == 1) {
        ScalingVariantWidget* scalingVariantWidget = qobject_cast<ScalingVariantWidget*>(ui->scalingVariantsGroupBox->layout()->itemAt(0)->widget());
        if (scalingVariantWidget) {
           scalingVariantWidget->setRemoveEnabled(false);
        }
    } else{
        setProjectDirty();
    }
}

void MainWindow::removeScalingVariant() {
    qDebug() << "remove" << sender();
    ScalingVariantWidget* scalingVariantWidget = qobject_cast<ScalingVariantWidget*>(sender());
    if (scalingVariantWidget) {
        disconnect(scalingVariantWidget, SIGNAL(valueChanged()), this, SLOT(scalingVariantWidgetValueChanged()));
        ui->scalingVariantsGroupBox->layout()->removeWidget(scalingVariantWidget);
        delete scalingVariantWidget;
    }

    setProjectDirty();
//    QAction* senderAction = dynamic_cast<QAction*>(sender());
}

void MainWindow::propertiesValueChanged() {
    if (_blockUISignals) return;
    QSettings settings;

    if (settings.value("MainWindow/automaticPreview", true).toBool()) {
        refreshAtlas();
    }
}

void MainWindow::on_displayOutlinesCheckBox_clicked(bool checked) {
    QSettings settings;
    settings.setValue("MainWindow/displayOutlines", checked);

    if (_outlinesGroup)
        _outlinesGroup->setVisible(checked);
}

void MainWindow::on_optModeComboBox_currentTextChanged(const QString &text) {
    bool visible = text == "Lossless";

    ui->optLevelSlider->setVisible(visible);
    ui->optLevelText->setVisible(visible);
    ui->optLevelLabel->setVisible(visible);

    setProjectDirty();
}

void MainWindow::on_optLevelSlider_valueChanged(int value) {
    ui->optLevelText->setText(QString::number(value));

    setProjectDirty();
}

void MainWindow::on_trimSpinBox_valueChanged(int value) {
    propertiesValueChanged();
    setProjectDirty();
}

void MainWindow::on_textureBorderSpinBox_valueChanged(int value) {
    propertiesValueChanged();
    setProjectDirty();
}

void MainWindow::on_spriteBorderSpinBox_valueChanged(int value) {
    propertiesValueChanged();
    setProjectDirty();
}

void MainWindow::on_epsilonHorizontalSlider_valueChanged(int value) {
    propertiesValueChanged();
    setProjectDirty();
}

void MainWindow::on_pow2ComboBox_currentIndexChanged(int value) {
    propertiesValueChanged();
    setProjectDirty();
}

void MainWindow::on_maxTextureSizeComboBox_currentTextChanged(const QString& text) {
    bool isValidSize;
    if (text.toInt(&isValidSize) > 8192) {
        isValidSize = false;
    }

    QPalette comboboxPalette = ui->maxTextureSizeComboBox->palette();
    if (!isValidSize) {
        comboboxPalette.setColor(QPalette::Text, Qt::red);
    } else {
        comboboxPalette.setColor(QPalette::Text, Qt::black);
    }
    ui->maxTextureSizeComboBox->setPalette(comboboxPalette);

    propertiesValueChanged();
    setProjectDirty();
}

void MainWindow::on_algorithmComboBox_currentTextChanged(const QString& text) {
    if (text == "Polygon") {
        ui->trimModeComboBox->setCurrentText("Polygon");
    }

    propertiesValueChanged();
    setProjectDirty();
}

void MainWindow::on_trimModeComboBox_currentIndexChanged(int value) {
    propertiesValueChanged();
    setProjectDirty();
}

void MainWindow::on_imageFormatComboBox_currentIndexChanged(int index) {
    auto setEnabledComboBoxItem = [](QComboBox* comboBox, int row, bool enable) {
        QStandardItemModel* model = qobject_cast<QStandardItemModel*>(comboBox->model());
        if (model) {
            QModelIndex index = model->index(row, comboBox->modelColumn());//, comboBox->rootModelIndex());
            QStandardItem* item = model->itemFromIndex(index);
            if (item) {
                item->setEnabled(enable);
            }
        }
    };
    ImageFormat imageFormat = (ImageFormat)index;
    if (imageFormat == kPNG) {
        ui->imageFormatStackedWidget->setVisible(true);
        ui->imageFormatStackedWidget->setCurrentIndex(0);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kRGB888, true);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kRGBA8888, true);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kETC1, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kPVRTC2, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kPVRTC2A, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kPVRTC4, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kPVRTC4A, false);
        if (ui->pixelFormatComboBox->currentIndex() > kRGBA8888) {
            ui->pixelFormatComboBox->setCurrentIndex(kRGBA8888);
        }
    } else if (imageFormat == kPKM) {
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kRGB888, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kRGBA8888, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kETC1, true);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kPVRTC2, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kPVRTC2A, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kPVRTC4, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kPVRTC4A, false);
        ui->pixelFormatComboBox->setCurrentIndex(kETC1);
    } else if ((imageFormat == kPVR)||(imageFormat == kPVR_CCZ)) {
        ui->imageFormatStackedWidget->setVisible(false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kRGB888, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kRGBA8888, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kETC1, false);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kPVRTC2, true);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kPVRTC2A, true);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kPVRTC4, true);
        setEnabledComboBoxItem(ui->pixelFormatComboBox, kPVRTC4A, true);
        if (ui->pixelFormatComboBox->currentIndex() < kPVRTC2) {
            ui->pixelFormatComboBox->setCurrentIndex(kPVRTC4);
        }
    }

    setProjectDirty();
}

void MainWindow::on_pixelFormatComboBox_currentIndexChanged(int index) {
    PixelFormat pixelFormat = (PixelFormat)index;
    if ((pixelFormat == kPVRTC2) ||
            (pixelFormat == kPVRTC2A) ||
            (pixelFormat == kPVRTC4) ||
            (pixelFormat == kPVRTC4A))
    {
        ui->pow2ComboBox->setCurrentIndex(1);
        ui->pow2ComboBox->setEnabled(false);
    } else {
        ui->pow2ComboBox->setEnabled(true);
    }
    setProjectDirty();
}

void MainWindow::on_dataFormatComboBox_currentIndexChanged(int value) {
    setProjectDirty();
}

void MainWindow::on_destPathLineEdit_textChanged(const QString& text) {
    setProjectDirty();
}

void MainWindow::on_spriteSheetLineEdit_textChanged(const QString& text) {
    setProjectDirty();
}

void MainWindow::scalingVariantWidgetValueChanged() {
    setProjectDirty();
}
