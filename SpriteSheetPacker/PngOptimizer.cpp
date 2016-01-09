#include "PngOptimizer.h"
#include "optipng.h"
#include <QtDebug>
#include <QtCore>

OptiPngOptimizer::OptiPngOptimizer(const QString& fileName, int optLevel) {
    _fileName = fileName;
    _optLevel = optLevel;

    opng_options options;
    memset(&options, 0, sizeof(options));
    options.optim_level = optLevel;
    options.interlace = -1;
    options.strip_all = 1;

    options.strategy_set |= (1U << 3);
    options.compr_level_set |= (1U << 9);
    options.mem_level_set |= (1U << 8);

    opng_ui ui;
    ui.printf_fn      = &OptiPngOptimizer::app_printf;
    ui.print_cntrl_fn = &OptiPngOptimizer::app_print_cntrl;
    ui.progress_fn    = &OptiPngOptimizer::app_progress;
    ui.panic_fn       = &OptiPngOptimizer::panic;
    if (opng_initialize(&options, &ui) != 0) {
        qCritical() << "Can't initialize optimization engine";
    }
}

OptiPngOptimizer::~OptiPngOptimizer() {
    if (opng_finalize() != 0) {
        qCritical() << "Can't finalize optimization engine";
    }
}

bool OptiPngOptimizer::optimize() {
    if (opng_optimize(_fileName.toStdString().c_str()) != 0) {
        qCritical() << "Error optimizing png";
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
