
//the NAND layout logical address that used FTL, and will be translated into nand_vector_t
typedef struct logcial_lun_t
{
    uint32          llun_offset          : MAX_LLUN_NR_BITS;
    uint32          llun_nand_type       : MAX_CELL_BITS;
    uint32          llun_spb_id          : MAX_BLOCK_PER_PLANE_BITS;
    uint32          rsvd1                : (32 - MAX_LLUN_NR_BITS - MAX_CELL_BITS - MAX_BLOCK_PER_PLANE_BITS);
    uint64          au_allocated_ptr     : MAX_AU_PER_LUN_BITS;
    uint64          au_allocated_nr      : MAX_AU_PER_LUN_BITS + 1;
    uint64          rsvd2                : (64 - 2 * MAX_AU_PER_LUN_BITS - 1);
    struct logcial_lun_t *next;
    nand_vector_t   *nand_vector;
    uint16          vector_cnt;
    uint8           *buf;
    void            *simulator_ptr;
}logcial_lun_t;
#define LOGICAL_LUN_OP_TYPE_READ    0
#define LOGICAL_LUN_OP_TYPE_PROGRAM 1
typedef struct logcial_lun_operator_t
{
    logcial_lun_t  *list;
    logcial_lun_t  start;
    uint16         cnt;
    uint16         op_type;
    void           *simulator_ptr;
}logcial_lun_operator_t;
