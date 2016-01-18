#include "PngOptimizer.h"
#include <QtDebug>
#include <QtCore>
#include "lodepng.h"
#include <QImage>

OptiPngOptimizer::OptiPngOptimizer(int optLevel) {
    _optLevel = optLevel;

    memset(&options, 0, sizeof(options));

    options.optim_level = _optLevel;
    options.interlace = -1;

    optimizer = opng_create_optimizer();
    transformer = opng_create_transformer();

    opng_set_options(optimizer, &options);

    opng_set_transformer(optimizer, transformer);
}

OptiPngOptimizer::~OptiPngOptimizer() {
    opng_destroy_optimizer(optimizer);
    opng_destroy_transformer(transformer);
}

bool OptiPngOptimizer::optimizeFiles(const QStringList& fileNames) {

    for(const QString& fileName : fileNames) {
        if (!optimizeFile(fileName)) {
            continue;
        }
    }

    return true;
}

bool OptiPngOptimizer::optimizeFile(const QString& fileName) {

    if (opng_optimize_file(optimizer,
                           fileName.toStdString().c_str(),
                           fileName.toStdString().c_str(),
                           NULL) == -1) {
        return false;
    }

    return true;
}

bool OptiPngOptimizer::setOptions(int optLevel) {
    _optLevel = optLevel;

    options.optim_level = _optLevel;

    if (opng_set_options(optimizer, &options) < 0) {
        return false;
    }

    return true;
}

PngQuantOptimizer::PngQuantOptimizer(int optLevel) {
    _optLevel = optLevel;

    attr = liq_attr_create();
}

PngQuantOptimizer::~PngQuantOptimizer() {
    liq_attr_destroy(attr);

    if (image) {
        liq_image_destroy(image);
    }

    if (res) {
        liq_result_destroy(res);
    }
}

bool PngQuantOptimizer::optimizeFiles(const QStringList& fileNames) {

    for(const QString& fileName : fileNames) {
        if (!optimizeFile(fileName)) {
            continue;
        }
    }

    return true;
}

bool PngQuantOptimizer::optimizeFile(const QString& fileName) {
    unsigned int width, height = 0;
    unsigned char *compressed = NULL;
    size_t compressed_size = 0;

    QImage img(fileName);
    img = img.convertToFormat(QImage::Format_RGBA8888);

    width = img.width();
    height = img.height();

    LodePNGState state;
    lodepng_state_init(&state);

    image = liq_image_create_rgba(attr, img.bits(), width, height, 0);
    liq_set_speed(attr, 1);

    res = liq_quantize_image(attr, image);

    if (!res) return false;

    liq_set_dithering_level(res, 1.0f);

    int buffer_size = width * height;
    unsigned char* buffer = (unsigned char*)malloc(buffer_size);

    if (liq_write_remapped_image(res, image, buffer, buffer_size) != LIQ_OK) {
        free(buffer);

        return false;
    }

    const liq_palette* pal = liq_get_palette(res);

    state.info_raw.colortype = LCT_PALETTE;
    state.info_raw.bitdepth = 8;
    state.info_png.color.colortype = LCT_PALETTE;
    state.info_png.color.bitdepth = pal->count <= 16 ? 4 : 8;
    state.encoder.auto_convert = 0;

    // png compression
    state.encoder.add_id = false;
    state.encoder.zlibsettings.nicematch = 258;
    state.encoder.zlibsettings.lazymatching = 1;
    state.encoder.zlibsettings.windowsize = 32768;

    for(unsigned int i = 0; i < pal->count; i++) {
        lodepng_palette_add(&state.info_png.color, pal->entries[i].r, pal->entries[i].g, pal->entries[i].b, pal->entries[i].a);
        lodepng_palette_add(&state.info_raw, pal->entries[i].r, pal->entries[i].g, pal->entries[i].b, pal->entries[i].a);
    }

    lodepng_encode(&compressed, &compressed_size, buffer, width, height, &state);
    lodepng_save_file(compressed, compressed_size, fileName.toStdString().c_str());

    lodepng_state_cleanup(&state);
    free(buffer);
    free(compressed);

    return true;
}

bool PngQuantOptimizer::setOptions(int optLevel) {
    _optLevel = optLevel;

    return true;
}
