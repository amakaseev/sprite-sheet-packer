#include "PngOptimizer.h"
#include "opnglib/include/opnglib/opnglib.h"
#include <QtDebug>
#include <QtCore>

static struct opng_options options;
static opng_optimizer_t *the_optimizer;
static opng_transformer_t *the_transformer;

OptiPngOptimizer::OptiPngOptimizer(const QString& fileName, int optLevel) {
    _fileName = fileName;
    _optLevel = optLevel;

/*
    opng_options options;
    memset(&options, 0, sizeof(options));
    options.optim_level = optLevel;
    options.force = 1;
    //options.use_stdout = 1;
    //options.interlace = -1;

    optimizer = opng_create_optimizer();
    transformer = opng_create_transformer();

    opng_set_options(optimizer, &options);
    opng_set_transformer(optimizer, transformer);
    */
}

OptiPngOptimizer::~OptiPngOptimizer() {
    opng_destroy_optimizer(optimizer);
}

bool OptiPngOptimizer::optimize() {

    opng_options options;
    memset(&options, 0, sizeof(options));

    options.optim_level = _optLevel;
    options.interlace = 1;

    options.zstrategy_set |= (1U << 3);
    options.zcompr_level_set |= (1U << 9);
    options.zmem_level_set |= (1U << 8);

    the_optimizer = opng_create_optimizer();
    the_transformer = opng_create_transformer();

    size_t err_objname_offset, err_objname_length = 0;
    const char * err_message = "";

    if (opng_transform_strip_objects(the_transformer, "all",
                     &err_objname_offset, &err_objname_length,
                     &err_message) >= 0) {
        return false;
    }

    opng_set_options(the_optimizer, &options);
    opng_set_transformer(the_optimizer, the_transformer);

    if (opng_optimize_file(the_optimizer,
                           _fileName.toStdString().c_str(),
                           "",//_fileName.toStdString().c_str(),
                           NULL) == -1) {
        return false;
    }

    return true;
}

void OptiPngOptimizer::app_printf(const char *fmt, ...) {
//    va_list args;
//    va_start(args, fmt);
//    printf(fmt, args);
//    va_end(args);
}

void OptiPngOptimizer::app_print_cntrl(int cntrl_code) {
    // TODO: implement
}

void OptiPngOptimizer::app_progress(unsigned long current_step, unsigned long total_steps) {
    // TODO: implement
    //qDebug("current_step: %d (total_steps: %d)\n", (int)current_step, (int)total_steps);
}

void OptiPngOptimizer::panic(const char *msg) {
    /* Print the panic message to stderr and terminate abnormally. */
    qCritical("** INTERNAL ERROR: %s", msg);
}
