#ifndef IMAGEFORMAT_H
#define IMAGEFORMAT_H

#include <QImage>
#include <QPainter>

enum ImageFormat {
    kPNG = 0,
    kWEBP,
    kJPG,
    kJPG_PNG,
    kPKM,
    kPVR,
    kPVR_CCZ
};

enum PixelFormat {
    kARGB8888 = 0,
    kARGB8565,
    kARGB4444,
    kRGB888,
    kRGB565,
    kALPHA,
    kETC1,
    kETC2,
    kETC2A,
    kPVRTC2,
    kPVRTC2A,
    kPVRTC4,
    kPVRTC4A,
    kDXT1,
    kDXT3,
    kDXT5
};

inline QString imageFormatToString(ImageFormat imageFormat) {
    switch (imageFormat) {
        case kPNG: return "*.png";
        case kWEBP: return "*.webp";
        case kJPG: return "*.jpg";
        case kJPG_PNG: return "(*.jpg) + (*.png)";
        case kPKM: return "*.pkm";
        case kPVR: return "*.pvr";
        case kPVR_CCZ: return "*.pvr.ccz";
        default: return "*.png";
    }
}

inline ImageFormat imageFormatFromString(const QString& imageFormat) {
    if (imageFormat == "*.png") return kPNG;
    if (imageFormat == "*.webp") return kWEBP;
    if (imageFormat == "*.jpg") return kJPG;
    if (imageFormat == "(*.jpg) + (*.png)") return kJPG_PNG;
    if (imageFormat == "*.pkm") return kPKM;
    if (imageFormat == "*.pvr") return kPVR;
    if (imageFormat == "*.pvr.ccz") return kPVR_CCZ;
    return kPNG;
}

inline QString pixelFormatToString(PixelFormat pixelFormat) {
    switch (pixelFormat) {
        case kARGB8888: return "ARGB8888";
        case kARGB8565: return "ARGB8565";
        case kARGB4444: return "ARGB4444";
        case kRGB888: return "RGB888";
        case kRGB565: return "RGB565";
        case kALPHA: return "ALPHA";
        case kETC1: return "ETC1";
        case kETC2: return "ETC2";
        case kETC2A: return "ETC2A";
        case kPVRTC2: return "PVRTC2";
        case kPVRTC2A: return "PVRTC2A";
        case kPVRTC4: return "PVRTC4";
        case kPVRTC4A: return "PVRTC4A";
        case kDXT1: return "DXT1";
        case kDXT3: return "DXT3";
        case kDXT5: return "DXT5";
        default: return "ARGB8888";
    }
}

inline PixelFormat pixelFormatFromString(const QString& pixelFormat) {
    if (pixelFormat == "ARGB8888") return kARGB8888;
    if (pixelFormat == "ARGB8565") return kARGB8565;
    if (pixelFormat == "ARGB4444") return kARGB4444;
    if (pixelFormat == "RGB888") return kRGB888;
    if (pixelFormat == "RGB565") return kRGB565;
    if (pixelFormat == "ALPHA") return kALPHA;
    if (pixelFormat == "ETC1") return kETC1;
    if (pixelFormat == "ETC2") return kETC2;
    if (pixelFormat == "ETC2A") return kETC2A;
    if (pixelFormat == "PVRTC2") return kPVRTC2;
    if (pixelFormat == "PVRTC2A") return kPVRTC2A;
    if (pixelFormat == "PVRTC4") return kPVRTC4;
    if (pixelFormat == "PVRTC4A") return kPVRTC4A;
    if (pixelFormat == "DXT1") return kDXT1;
    if (pixelFormat == "DXT3") return kDXT3;
    if (pixelFormat == "DXT5") return kDXT5;
    return kARGB8888;
}

inline QImage convertImage(const QImage& image, PixelFormat pixelFormat, bool premultiplied) {
    switch (pixelFormat) {
        case kRGB888: return image.convertToFormat(QImage::Format_RGB888);
        case kRGB565: return image.convertToFormat(QImage::Format_RGB16);
        case kALPHA: {
            QImage alphaImage = image.convertToFormat(QImage::Format_Alpha8);
            QImage mono = image;
            mono.fill(QColor(Qt::white).rgb());
            QPainter painter(&mono);
            painter.drawImage(0, 0, alphaImage);
            mono.invertPixels(QImage::InvertRgb);
            return mono.convertToFormat(QImage::Format_Grayscale8);
        }
        default: break;
    }
    if (premultiplied) {
        switch (pixelFormat) {
            case kARGB8888: return image.convertToFormat(QImage::Format_ARGB32);
            case kARGB8565: return image.convertToFormat(QImage::Format_ARGB8565_Premultiplied);
            case kARGB4444: return image.convertToFormat(QImage::Format_ARGB4444_Premultiplied);
            default: break;
        }
    } else {
        switch (pixelFormat) {
            case kARGB8888: return image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
            case kARGB8565: return image.convertToFormat(QImage::Format_ARGB8565_Premultiplied);
            case kARGB4444: return image.convertToFormat(QImage::Format_ARGB4444_Premultiplied);
            default: break;
        }
    }
    return image;
}

#endif // IMAGEFORMAT_H
