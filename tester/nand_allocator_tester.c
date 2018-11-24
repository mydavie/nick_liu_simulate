#include "../lib_header.h"
#include "../nand_vectors/nand_vectors.h"
#include "../nand_virtualize/logical_lun_operator.h"
#include "../nand_allocator/nand_allocator.h"
#include "nand_allocator_tester.h"

void nand_allocator_tester(mongoc_gridfs_t *gridfs)
{
    logical_lun_reader_t logical_lun_reader;
    logical_lun_writer_t logcial_lun_writer;
    memset(&logical_lun_reader, 0, sizeof (logical_lun_reader_t));
    memset(&logcial_lun_writer, 0, sizeof (logical_lun_writer_t));
    for (uint32 i = 0; i < 100; i++) {
        memset(&logcial_lun_writer, 0, sizeof (logical_lun_writer_t));
        logcial_lun_writer.plut         = get_lut_ptr(0);
        logcial_lun_writer.laa_start    = 0;
        logcial_lun_writer.laa_cnt      = 48;
        logcial_lun_writer.simulator    = gridfs;
        laa_range_write(&logcial_lun_writer);
    }

    logical_lun_reader.plut         = get_lut_ptr(0);
    logical_lun_reader.laa_start    = 0;
    logical_lun_reader.laa_cnt      = 1;
    logical_lun_reader.simulator    = gridfs;
    laa_range_read(&logical_lun_reader);
}
