#include "PngOptimizer.h"
#include "opnglib/include/opnglib/opnglib.h"
#include <QtDebug>
#include <QtCore>

OptiPngOptimizer::OptiPngOptimizer(const QString& fileName, int optLevel) {
    _fileName = fileName;
    _optLevel = optLevel;

    memset(&options, 0, sizeof(options));

    options.optim_level = _optLevel;
    options.interlace = -1;

    options.zstrategy_set |= (1U << 3);
    options.zcompr_level_set |= (1U << 9);
    options.zmem_level_set |= (1U << 8);

    optimizer = opng_create_optimizer();
    transformer = opng_create_transformer();
}

OptiPngOptimizer::~OptiPngOptimizer() {
    opng_destroy_optimizer(optimizer);
    opng_destroy_transformer(transformer);
}

bool OptiPngOptimizer::optimize() {

    size_t err_objname_offset, err_objname_length = 0;
    const char * err_message = "";

    opng_set_options(optimizer, &options);
    opng_set_transformer(optimizer, transformer);

    //if (opng_transform_strip_objects(transformer, "all",
    //                 &err_objname_offset, &err_objname_length,
    //                 &err_message) >= 0) {
    //    return false;
    //}

    if (opng_optimize_file(optimizer,
                           _fileName.toStdString().c_str(),
                           _fileName.toStdString().c_str(),
                           NULL) == -1) {
        return false;
    }

    return true;
}
