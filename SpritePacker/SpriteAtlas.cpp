#include "SpriteAtlas.h"

#include "binpack2d.hpp"
#include "ImageRotate.h"

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
};

SpriteAtlas::SpriteAtlas(const QStringList& sourceList, int textureBorder, int spriteBorder, int trim, float scale)
    : _sourceList(sourceList)
    , _textureBorder(textureBorder)
    , _spriteBorder(spriteBorder)
    , _trim(trim)
    , _scale(scale)
{

}

// TODO: QThread: gui is freeze on very large atlases (no profit) wtf?
//void SpriteAtlas::generateOnThread() {
//    QThread* generateThread = new QThread(this);

//     connect(generateThread, SIGNAL(started()), this, SLOT(generate()));
//     connect(generateThread, SIGNAL(finished()), this, SLOT(deleteLater()));

//     generateThread->start();
//}

void SpriteAtlas::generate() {
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
                //qDebug() << fileList.back();
            }
        } else {
            fileList.push_back(qMakePair(pathName, fi.fileName()));
            //qDebug() << fileList.back();
        }
    }

    int volume = 0;

    QMap<QString, QVector<QPair<QString, QSize>>> identicalContent;
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
        }

        bool findIdentical = false;
        auto& contentVector = inputContent.Get();
        for (auto& content: contentVector) {
            if (content.content.isIdentical(packContent)) {
                findIdentical = true;
                identicalContent[content.content.mName].push_back(qMakePair(packContent.mName, packContent.mImage.size()));
                qDebug() << "isIdentical";
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

    // Sort the input content by size... usually packs better.
    inputContent.Sort();

    // A place to store packed content.
    BinPack2D::ContentAccumulator<PackContent> remainder;
    BinPack2D::ContentAccumulator<PackContent> outputContent;

    // find optimal size for atlas
    int w = sqrt(volume) + _textureBorder*2;
    int h = sqrt(volume) + _textureBorder*2;
    qDebug() << w << "x" << h;
    bool k = true;
    int step = (w + h) / 20;
    while (1) {
        BinPack2D::CanvasArray<PackContent> canvasArray = BinPack2D::UniformCanvasArrayBuilder<PackContent>(w - _textureBorder*2, h - _textureBorder*2, 1).Build();

        bool success = canvasArray.Place(inputContent, remainder);
        if (success) {
            outputContent = BinPack2D::ContentAccumulator<PackContent>();
            canvasArray.CollectContent(outputContent);
            break;
        }
        if (k) {
            k = false;
            w += step;
        } else {
            k = true;
            h += step;
        }
        qDebug() << "stage 1:" << w << "x" << h << "step:" << step;
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
        qDebug() << "stage 2:" << w << "x" << h << "step:" << step;
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
        qDebug() << "stage 3:" << w << "x" << h << "step:" << step;
        QCoreApplication::processEvents();
    }


    // parse output.
    typedef BinPack2D::Content<PackContent>::Vector::iterator binpack2d_iterator;

    _atlasImage = QImage(w, h, QImage::Format_RGBA8888);
    _atlasImage.fill(QColor(0, 0, 0, 0));

    _spriteFrames.clear();
    int skipSprites = 0;
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
        auto identicalIt = identicalContent.find(packContent.mName);
        if (identicalIt != identicalContent.end()) {
            QStringList identicalList;
            for (auto ident: (*identicalIt)) {
                SpriteFrameInfo identSpriteFrameInfo = spriteFrame;
                identSpriteFrameInfo.mOffset = QPoint(
                            (packContent.mRect.left() + (-ident.second.width() + content.size.w - _spriteBorder) * 0.5f),
                            (-packContent.mRect.top() + ( ident.second.height() - content.size.h + _spriteBorder) * 0.5f)
                            );
                identSpriteFrameInfo.mSourceSize = ident.second;
                _spriteFrames[ident.first] = identSpriteFrameInfo;

                identicalList.push_back(ident.first);
            }
            skipSprites += identicalList.size();
            qDebug() << packContent.mName << "identical:" << identicalList;
        }
    }

    qDebug() << "Total skip sprites: " << skipSprites;
    int elapsed = timePerform.elapsed();
    qDebug() << "Generate time mc:" <<  elapsed << " sec:" << elapsed/1000.f;
}
