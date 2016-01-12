#include "SpriteAtlas.h"

#include "binpack2d.hpp"
#include "ImageRotate.h"
#include "PolygonImage.h"

int pow2(int len) {
    int order = 1;
    while(pow(2,order) < len)
    {
       order++;
    }
    return pow(2,order);
}

void copyImage(QImage& dst, const QPoint& pos, const QImage& src, const QRect& srcRect) {
    for (int x=0; x<srcRect.width(); ++x) {
        for (int y=0; y<srcRect.height(); ++y) {
            dst.setPixel(pos.x() + x, pos.y() + y, src.pixel(srcRect.x() + x, srcRect.y() + y));
        }
    }
}

PackContent::PackContent() {
    // only for QVector
    qDebug() << "PackContent::PackContent()";
}
PackContent::PackContent(const QString& name, const QImage& image) {
    _name = name;
    _image = image;
    _rect = QRect(0, 0, _image.width(), _image.height());
}

bool PackContent::isIdentical(const PackContent& other) {
    if (_rect != other._rect) return false;

    for (int x = _rect.left(); x < _rect.right(); ++x) {
        for (int y = _rect.top(); y < _rect.bottom(); ++y) {
            if (_image.pixel(x, y) != other._image.pixel(x, y)) return false;
        }
    }

    return true;
}

void PackContent::trim(int alpha) {
    int l = _image.width();
    int t = _image.height();
    int r = 0;
    int b = 0;
    for (int y=0; y<_image.height(); y++) {
        bool rowFilled = false;
        for (int x=0; x<_image.width(); x++) {
            int a = qAlpha(_image.pixel(x, y));
            if (a >= alpha) {
                rowFilled = true;
                r = qMax(r, x);
                if (l > x) {
                    l = x;
                }
            }
        }
        if (rowFilled) {
            t = qMin(t, y);
            b = y;
        }
    }
    _rect = QRect(QPoint(l, t), QPoint(r,b));
    if ((_rect.width() % 2) != (_image.width() % 2)) {
        if (l>0) l--; else r++;
        _rect = QRect(QPoint(l, t), QPoint(r,b));
    }
    if ((_rect.height() % 2) != (_rect.height() % 2)) {
        if (t>0) t--; else b++;
        _rect = QRect(QPoint(l, t), QPoint(r,b));
    }
    if ((_rect.width()<0)||(_rect.height()<0)) {
        _rect = QRect(0, 0, 2, 2);
    }
}

SpriteAtlas::SpriteAtlas(const QStringList& sourceList, int textureBorder, int spriteBorder, int trim, bool pow2, int maxSize, float scale)
    : _sourceList(sourceList)
    , _trim(trim)
    , _textureBorder(textureBorder)
    , _spriteBorder(spriteBorder)
    , _pow2(pow2)
    , _maxTextureSize(maxSize)
    , _scale(scale)
{
    _algorithm = "Rect";
    _polygonMode.enable = false;
}

void SpriteAtlas::enablePolygonMode(bool enable, float epsilon) {
    _polygonMode.enable = enable;
    _polygonMode.epsilon = epsilon;
}

// TODO: QThread: gui is freeze on very large atlases (no profit) wtf?
//void SpriteAtlas::generateOnThread() {
//    QThread* generateThread = new QThread(this);

//     connect(generateThread, SIGNAL(started()), this, SLOT(generate()));
//     connect(generateThread, SIGNAL(finished()), this, SLOT(deleteLater()));

//     generateThread->start();
//}

bool SpriteAtlas::generate() {
    QTime timePerform;
    timePerform.start();

    QStringList nameFilter;
    nameFilter << "*.png" << "*.jpg" << "*.jpeg" << "*.gif" << "*.bmp";

    QList< QPair<QString, QString> > fileList;
    for(auto pathName: _sourceList) {
        QFileInfo fi(pathName);

        if (fi.isDir()) {
            QDir dir(fi.path());
            QDirIterator fileNames(pathName, nameFilter, QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
            while(fileNames.hasNext()){
                fileNames.next();
                fileList.push_back(qMakePair(fileNames.filePath(), dir.relativeFilePath(fileNames.filePath())));
            }
        } else {
            fileList.push_back(qMakePair(pathName, fi.fileName()));
        }
    }

    int skipSprites = 0;

    // init images and rects
    _identicalFrames.clear();

    QVector<PackContent> inputContent;
    auto it_f = fileList.begin();
    for(; it_f != fileList.end(); ++it_f) {
        QImage image((*it_f).first);
        if (image.isNull()) continue;
        if (_scale != 1) {
            image = image.scaled(ceil(image.width() * _scale), ceil(image.height() * _scale), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        PackContent packContent((*it_f).second, image);

        // Trim / Crop
        if (_trim) {
            packContent.trim(_trim);
            if (_polygonMode.enable) {
                packContent.setTriangles(PolygonImage::generateTriangles(packContent.image(), packContent.rect(), _polygonMode.epsilon, _trim));
            }
        }

        // Find Identical
        bool findIdentical = false;
        for (auto& content: inputContent) {
            if (content.isIdentical(packContent)) {
                findIdentical = true;
                _identicalFrames[content.name()].push_back(packContent.name());
                qDebug() << "isIdentical:" << packContent.name() << "==" << content.name();
                skipSprites++;
                break;
            }
        }
        if (findIdentical) {
            continue;
        }

        inputContent.push_back(packContent);
    }
    if (skipSprites)
        qDebug() << "Total skip sprites: " << skipSprites;

    QCoreApplication::processEvents();

    bool result = false;
    if ((_algorithm == "Polygon") && (_polygonMode.enable)) {
        result = packWithPolygon(inputContent);
    } else {
        result = packWithRect(inputContent);
    }

    int elapsed = timePerform.elapsed();
    qDebug() << "Generate time mc:" <<  elapsed/1000.f << "sec";

    return result;
}

bool SpriteAtlas::packWithRect(const QVector<PackContent>& content) {
    int volume = 0;
    BinPack2D::ContentAccumulator<PackContent> inputContent;
    for (auto packContent: content) {
        int width = packContent.rect().width();
        int height = packContent.rect().height();
        volume += width * height * 1.02f;

        inputContent += BinPack2D::Content<PackContent>(packContent,
                                                        BinPack2D::Coord(),
                                                        BinPack2D::Size(width + _spriteBorder, height + _spriteBorder),
                                                        false);
    }

    // Sort the input content by size... usually packs better.
    inputContent.Sort();

    // A place to store packed content.
    BinPack2D::ContentAccumulator<PackContent> remainder;
    BinPack2D::ContentAccumulator<PackContent> outputContent;

    // find optimal size for atlas
    int w = qMin(_maxTextureSize, (int)sqrt(volume));
    int h = qMin(_maxTextureSize, (int)sqrt(volume));
    if (_pow2) {
        w = pow2(w);
        h = pow2(h);
        qDebug() << "Volume size:" << w << "x" << h;

        bool k = true;
        while (1) {
            BinPack2D::CanvasArray<PackContent> canvasArray = BinPack2D::UniformCanvasArrayBuilder<PackContent>(w - _textureBorder*2, h - _textureBorder*2, 1).Build();

            bool success = canvasArray.Place(inputContent, remainder);
            if (success) {
                outputContent = BinPack2D::ContentAccumulator<PackContent>();
                canvasArray.CollectContent(outputContent);
                break;
            } else {
                if ((w == _maxTextureSize) && (h == _maxTextureSize)) {
                    qDebug() << "Max size Limit!";
                    return false;
                }
            }
            if (k) {
                k = false;
                w = qMin(w*2, _maxTextureSize);
            } else {
                k = true;
                h = qMin(h*2, _maxTextureSize);
            }
            qDebug() << "Resize for bigger:" << w << "x" << h;
            QCoreApplication::processEvents();
        }
        while (w > 2) {
            w = w/2;
            BinPack2D::CanvasArray<PackContent> canvasArray = BinPack2D::UniformCanvasArrayBuilder<PackContent>(w - _textureBorder*2, h - _textureBorder*2, 1).Build();

            bool success = canvasArray.Place(inputContent, remainder);
            if (!success) {
                w = w*2;
                break;
            } else {
                outputContent = BinPack2D::ContentAccumulator<PackContent>();
                canvasArray.CollectContent(outputContent);
            }
            qDebug() << "Optimize width:" << w << "x" << h;
            QCoreApplication::processEvents();
        }
        while (h > 2) {
            h = h/2;
            BinPack2D::CanvasArray<PackContent> canvasArray = BinPack2D::UniformCanvasArrayBuilder<PackContent>(w - _textureBorder*2, h - _textureBorder*2, 1).Build();

            bool success = canvasArray.Place(inputContent, remainder);
            if (!success) {
                h = h*2;
                break;
            } else {
                outputContent = BinPack2D::ContentAccumulator<PackContent>();
                canvasArray.CollectContent(outputContent);
            }
            qDebug() << "Optimize height:" << w << "x" << h;
            QCoreApplication::processEvents();
        }
    } else {
        qDebug() << "Volume size:" << w << "x" << h;
        bool k = true;
        int step = (w + h) / 20;
        while (1) {
            BinPack2D::CanvasArray<PackContent> canvasArray = BinPack2D::UniformCanvasArrayBuilder<PackContent>(w - _textureBorder*2, h - _textureBorder*2, 1).Build();

            bool success = canvasArray.Place(inputContent, remainder);
            if (success) {
                outputContent = BinPack2D::ContentAccumulator<PackContent>();
                canvasArray.CollectContent(outputContent);
                break;
            } else {
                if ((w == _maxTextureSize) && (h == _maxTextureSize)) {
                    qDebug() << "Max size Limit!";
                    return false;
                }
            }
            if (k) {
                k = false;
                w = qMin(w + step, _maxTextureSize);
            } else {
                k = true;
                h = qMin(h + step, _maxTextureSize);;
            }
            qDebug() << "Resize for bigger:" << w << "x" << h << "step:" << step;
            QCoreApplication::processEvents();
        }
        step = (w + h) / 20;
        while (w) {
            w -= step;
            BinPack2D::CanvasArray<PackContent> canvasArray = BinPack2D::UniformCanvasArrayBuilder<PackContent>(w - _textureBorder*2, h - _textureBorder*2, 1).Build();

            bool success = canvasArray.Place(inputContent, remainder);
            if (!success) {
                w += step;
                if (step > 1) step = qMax(step/2, 1); else break;
            } else {
                outputContent = BinPack2D::ContentAccumulator<PackContent>();
                canvasArray.CollectContent(outputContent);
            }
            qDebug() << "Optimize width:" << w << "x" << h << "step:" << step;
            QCoreApplication::processEvents();
        }
        step = (w + h) / 20;
        while (h) {
            h -= step;
            BinPack2D::CanvasArray<PackContent> canvasArray = BinPack2D::UniformCanvasArrayBuilder<PackContent>(w - _textureBorder*2, h - _textureBorder*2, 1).Build();

            bool success = canvasArray.Place(inputContent, remainder);
            if (!success) {
                h += step;
                if (step > 1) step = qMax(step/2, 1); else break;
            } else {
                outputContent = BinPack2D::ContentAccumulator<PackContent>();
                canvasArray.CollectContent(outputContent);
            }
            qDebug() << "Optimize height:" << w << "x" << h << "step:" << step;
            QCoreApplication::processEvents();
        }
    }

    qDebug() << "Found optimize size:" << w << "x" << h;
    QCoreApplication::processEvents();

    // parse output.
    _atlasImage = QImage(w, h, QImage::Format_RGBA8888);
    _atlasImage.fill(QColor(0, 0, 0, 0));
    _spriteFrames.clear();
    for(auto itor = outputContent.Get().begin(); itor != outputContent.Get().end(); itor++ ) {
        const BinPack2D::Content<PackContent> &content = *itor;

        // retreive your data.
        const PackContent &packContent = content.content;
        //qDebug() << packContent.mName << packContent.mRect;

        // image
        QImage image;
        if (content.rotated) {
            image = packContent.image().copy(packContent.rect());
            image = rotate90(image);
        }

        SpriteFrameInfo spriteFrame;
        spriteFrame.triangles = packContent.triangles();
        spriteFrame.frame = QRect(content.coord.x + _textureBorder, content.coord.y + _textureBorder, content.size.w - _spriteBorder, content.size.h - _spriteBorder);
        spriteFrame.offset = QPoint(
                    (packContent.rect().left() + (-packContent.image().width() + content.size.w - _spriteBorder) * 0.5f),
                    (-packContent.rect().top() + ( packContent.image().height() - content.size.h + _spriteBorder) * 0.5f)
                    );
        spriteFrame.rotated = content.rotated;
        spriteFrame.sourceColorRect = packContent.rect();
        spriteFrame.sourceSize = packContent.image().size();
        if (content.rotated) {
            spriteFrame.frame = QRect(content.coord.x, content.coord.y, content.size.h-_spriteBorder, content.size.w-_spriteBorder);

        }
        if (content.rotated) {
            copyImage(_atlasImage, QPoint(content.coord.x+_textureBorder, content.coord.y+_textureBorder), image, image.rect());
        } else {
            copyImage(_atlasImage, QPoint(content.coord.x+_textureBorder, content.coord.y+_textureBorder), packContent.image(), packContent.rect());
        }
        _spriteFrames[packContent.name()] = spriteFrame;

        // add ident to sprite frames
        auto identicalIt = _identicalFrames.find(packContent.name());
        if (identicalIt != _identicalFrames.end()) {
            QStringList identicalList;
            for (auto ident: (*identicalIt)) {
                SpriteFrameInfo identSpriteFrameInfo = spriteFrame;
                identSpriteFrameInfo.offset = QPoint(
                            (packContent.rect().left() + (-packContent.image().width() + content.size.w - _spriteBorder) * 0.5f),
                            (-packContent.rect().top() + ( packContent.image().height() - content.size.h + _spriteBorder) * 0.5f)
                            );
                identSpriteFrameInfo.sourceSize = packContent.image().size();
                _spriteFrames[ident] = identSpriteFrameInfo;

                identicalList.push_back(ident);
            }
        }
    }

    return true;
}

bool SpriteAtlas::packWithPolygon(const QVector<PackContent>& content) {

    return true;
}
