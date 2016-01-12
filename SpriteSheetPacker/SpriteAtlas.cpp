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

class PackContent {
public:
    PackContent(const QString& fname, const QString& name, const QImage& image) {
        mFileName = fname;
        mName = name;
        mImage = image;
        mRect = QRect(0, 0, mImage.width(), mImage.height());
    }

    bool isIdentical(const PackContent& other) {
        if (mRect != other.mRect) return false;

        for (int x=mRect.left(); x<=mRect.right(); ++x) {
            for (int y=mRect.top(); y<=mRect.bottom(); ++y) {
                if (mImage.pixel(x, y) != other.mImage.pixel(x, y)) return false;
            }
        }

        return true;
    }

    void trim(int alpha) {
        int l = mImage.width();
        int t = mImage.height();
        int r = 0;
        int b = 0;
        for (int y=0; y<mImage.height(); y++) {
            bool rowFilled = false;
            for (int x=0; x<mImage.width(); x++) {
                int a = qAlpha(mImage.pixel(x, y));
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
        mRect = QRect(QPoint(l, t), QPoint(r,b));
        if ((mRect.width() % 2) != (mImage.width() % 2)) {
            if (l>0) l--; else r++;
            mRect = QRect(QPoint(l, t), QPoint(r,b));
        }
        if ((mRect.height() % 2) != (mImage.height() % 2)) {
            if (t>0) t--; else b++;
            mRect = QRect(QPoint(l, t), QPoint(r,b));
        }
        if ((mRect.width()<0)||(mRect.height()<0)) {
            mRect = QRect(0, 0, 2, 2);
        }
    }

    QString mFileName;
    QString mName;
    QImage  mImage;
    QRect   mRect;
    Triangles triangles;
};

SpriteAtlas::SpriteAtlas(const QStringList& sourceList, int textureBorder, int spriteBorder, int trim, bool pow2, int maxSize, float scale)
    : _sourceList(sourceList)
    , _textureBorder(textureBorder)
    , _spriteBorder(spriteBorder)
    , _trim(trim)
    , _pow2(pow2)
    , _maxTextureSize(maxSize)
    , _scale(scale)
{
    _polygonMode.enable = false;
}

bool SpriteAtlas::enablePolygonMode(bool enable, float epsilon) {
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

    BinPack2D::ContentAccumulator<PackContent> inputContent;

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

    int volume = 0;
    int skipSprites = 0;
    _identicalFrames.clear();
    // init images and rects
    QList< QPair<QString,QString> >::iterator it_f = fileList.begin();
    for(; it_f != fileList.end(); ++it_f) {
        QImage image((*it_f).first);
        if (image.isNull()) continue;
        if (_scale != 1) {
            image = image.scaled(ceil(image.width() * _scale), ceil(image.height() * _scale), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        PackContent packContent((*it_f).first, (*it_f).second, image);

        // Trim / Crop
        if (_trim) {
            packContent.trim(_trim);
            if (_polygonMode.enable) {
                packContent.triangles = PolygonImage::generateTriangles(
                            packContent.mImage,
                            packContent.mRect,
                            _polygonMode.epsilon,
                            _trim);
            }
        }

        bool findIdentical = false;
        auto& contentVector = inputContent.Get();
        for (auto& content: contentVector) {
            if (content.content.isIdentical(packContent)) {
                findIdentical = true;
                _identicalFrames[content.content.mName].push_back(packContent.mName);
                qDebug() << "isIdentical:" << packContent.mName << "==" << content.content.mName;
                skipSprites++;
                break;
            }
        }

        if (findIdentical) {
            continue;
        }

        int width = packContent.mRect.width();
        int height = packContent.mRect.height();
        volume += width * height * 1.02f;

        inputContent += BinPack2D::Content<PackContent>(packContent,
                                                        BinPack2D::Coord(),
                                                        BinPack2D::Size(width+_spriteBorder, height+_spriteBorder),
                                                        false);
    }
    if (skipSprites)
        qDebug() << "Total skip sprites: " << skipSprites;

    QCoreApplication::processEvents();

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
    typedef BinPack2D::Content<PackContent>::Vector::iterator binpack2d_iterator;

    _atlasImage = QImage(w, h, QImage::Format_RGBA8888);
    _atlasImage.fill(QColor(0, 0, 0, 0));

    _spriteFrames.clear();
    for( binpack2d_iterator itor = outputContent.Get().begin(); itor != outputContent.Get().end(); itor++ ) {
        const BinPack2D::Content<PackContent> &content = *itor;

        // retreive your data.
        const PackContent &packContent = content.content;
        //qDebug() << packContent.mName << packContent.mRect;

        // image
        QImage image;
        if (content.rotated) {
            image = packContent.mImage.copy(packContent.mRect);
            image = rotate90(image);
        }

        SpriteFrameInfo spriteFrame;
        spriteFrame.triangles = packContent.triangles;
        spriteFrame.mFrame = QRect(content.coord.x + _textureBorder, content.coord.y + _textureBorder, content.size.w-_spriteBorder, content.size.h-_spriteBorder);
        spriteFrame.mOffset = QPoint(
                    (packContent.mRect.left() + (-packContent.mImage.width() + content.size.w - _spriteBorder) * 0.5f),
                    (-packContent.mRect.top() + ( packContent.mImage.height() - content.size.h + _spriteBorder) * 0.5f)
                    );
        spriteFrame.mRotated = content.rotated;
        spriteFrame.mSourceColorRect = packContent.mRect;
        spriteFrame.mSourceSize = packContent.mImage.size();
        if (content.rotated) {
            spriteFrame.mFrame = QRect(content.coord.x, content.coord.y, content.size.h-_spriteBorder, content.size.w-_spriteBorder);

        }
        if (content.rotated) {
            copyImage(_atlasImage, QPoint(content.coord.x+_textureBorder, content.coord.y+_textureBorder), image, image.rect());
        } else {
            copyImage(_atlasImage, QPoint(content.coord.x+_textureBorder, content.coord.y+_textureBorder), packContent.mImage, packContent.mRect);
        }
        _spriteFrames[packContent.mName] = spriteFrame;

        // add ident to sprite frames
        auto identicalIt = _identicalFrames.find(packContent.mName);
        if (identicalIt != _identicalFrames.end()) {
            QStringList identicalList;
            for (auto ident: (*identicalIt)) {
                SpriteFrameInfo identSpriteFrameInfo = spriteFrame;
                identSpriteFrameInfo.mOffset = QPoint(
                            (packContent.mRect.left() + (-packContent.mImage.width() + content.size.w - _spriteBorder) * 0.5f),
                            (-packContent.mRect.top() + ( packContent.mImage.height() - content.size.h + _spriteBorder) * 0.5f)
                            );
                identSpriteFrameInfo.mSourceSize = packContent.mImage.size();
                _spriteFrames[ident] = identSpriteFrameInfo;

                identicalList.push_back(ident);
            }
        }
    }

    int elapsed = timePerform.elapsed();
    qDebug() << "Generate time mc:" <<  elapsed/1000.f << "sec";
    return true;
}
