
//the NAND layout logical address that used FTL, and will be translated into nand_vector_t

typedef union _param_t
{
    struct _range_t
    {
        uint16 au_start;
        uint16 au_cnt;
    }range;
    struct _list_t
    {
        uint16 *au_list;
        uint16 au_cnt;
    }list;
}param_t;
typedef struct _logical_lun_t
{
    uint32          llun_offset          : MAX_LLUN_NR_BITS;
    uint32          llun_nand_type       : MAX_CELL_BITS;
    uint32          llun_spb_id          : MAX_BLOCK_PER_PLANE_BITS;
    uint32          rsvd1                : (32 - MAX_LLUN_NR_BITS - MAX_CELL_BITS - MAX_BLOCK_PER_PLANE_BITS);
    param_t         au_param;
    uint16          au_param_type;
    struct _logical_lun_t *next;
    nand_vector_t   *nand_vector;
    uint16          vector_cnt;
    uint8           *buf;
    void            *simulator_ptr;
}logical_lun_t;

#define LOGICAL_LUN_OP_TYPE_READ    0
#define LOGICAL_LUN_OP_TYPE_PROGRAM 1
#define LOGICAL_LUN_PARAM_RANGE     0
#define LOGICAL_LUN_PARAM_LIST      1

typedef struct logcial_lun_operator_t
{
    logical_lun_t  *list;
    logical_lun_t  start;
    uint16         cnt;
    uint16         op_type;
    void           *simulator_ptr;
}logcial_lun_operator_t;

uint32 submit_logical_lun_operator(logcial_lun_operator_t *plogcial_lun_operator);
logical_lun_t* logical_lun_allcoate(uint32 want_nr, uint32 *result_nr);
uint32 logical_lun_release(logical_lun_t* start, uint32 vector_cnt);
uint32 logical_lun_to_physical_lun(uint32 logical_lun_offset);
void logical_lun_init_onetime(void);


