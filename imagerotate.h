/* ImageRotate
Functions for image rotating for values of 90-degrees multiples
Copyright (C) 2005-2006 Wesley Crossman
Email: wesley@crossmans.net

You can redistribute and/or modify this software under the terms of the GNU
General Public License as published by the Free Software Foundation;
either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this
program; if not, write to the Free Software Foundation, Inc., 59 Temple Place,
Suite 330, Boston, MA 02111-1307 USA */

#ifndef IMAGE_ROTATE_H
#define IMAGE_ROTATE_H

#include <QPixmap>
#include <QImage>

QImage rotate(int degrees, const QImage &src);
QImage rotate90(const QImage &src);
QImage rotate180(const QImage &src);
QImage rotate270(const QImage &src);

QImage rotate(int degrees, const QImage &src) {
    if (degrees == 90) {
        return rotate90(src);
    } else if (degrees == 180) {
        return rotate180(src);
    } else if (degrees == 270 || degrees == -90) {
        return rotate270(src);
    } else if (degrees == 0 || degrees == 360) {
        return src;
    } else {
        return QImage();
    }
}
QImage rotate90(const QImage &src) {
    QImage dst(src.height(), src.width(), src.format());
    for (int y=0;y<src.height();++y) {
        const uint *srcLine = reinterpret_cast< const uint * >(src.scanLine(y));
        for (int x=0;x<src.width();++x) {
            dst.setPixel(src.height()-y-1, x, srcLine[x]);
        }
    }
    return dst;
}
QImage rotate180(const QImage &src) {
    QImage dst(src.width(), src.height(), src.format());
    for (int y=0;y<src.height();++y) {
        const uint *srcLine = reinterpret_cast< const uint * >(src.scanLine(y));
        for (int x=0;x<src.width();++x) {
            dst.setPixel(src.width()-x-1, src.height()-y-1, srcLine[x]);
        }
    }
    return dst;
}
QImage rotate270(const QImage &src) {
    QImage dst(src.height(), src.width(), src.format());
    for (int y=0;y<src.height();++y) {
        const uint *srcLine = reinterpret_cast< const uint * >(src.scanLine(y));
        for (int x=0;x<src.width();++x) {
            dst.setPixel(y, src.width()-x-1, srcLine[x]);
        }
    }
    return dst;
}

//convenience functions which convert from/to qpixmap
QPixmap rotate(int degrees, const QPixmap &src);
QPixmap rotate90(const QPixmap &src);
QPixmap rotate180(const QPixmap &src);
QPixmap rotate270(const QPixmap &src);
QPixmap rotate(int degrees, const QPixmap &src) {
    return QPixmap::fromImage(rotate(degrees, src.toImage()));
}
QPixmap rotate90(const QPixmap &src) {
    return QPixmap::fromImage(rotate90(src.toImage()));
}
QPixmap rotate180(const QPixmap &src) {
    return QPixmap::fromImage(rotate180(src.toImage()));
}
QPixmap rotate270(const QPixmap &src) {
    return QPixmap::fromImage(rotate270(src.toImage()));
}

#endif
