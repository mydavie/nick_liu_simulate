
typedef struct _paa_t
{
    uint64  spb_id          : MAX_BLOCK_PER_PLANE_BITS;
    uint64  logical_lun_id  : MAX_LLUN_NR_BITS;
    uint64  au_ptr_of_lun   : MAX_AU_PER_LUN_BITS;
    uint64  rsvd            : (64 - MAX_BLOCK_PER_PLANE_BITS - MAX_AU_PER_LUN_BITS - MAX_LLUN_NR_BITS);
}paa_t;

typedef struct _lut_t
{
    paa_t paa[100];
    uint32 lut_size;
    uint32 paa_cnt;
}lut_t;


typedef struct _spb_info_t
{
    uint8   pb_type : MAX_CELL_BITS;
    uint16  erase_cnt;
    uint32  read_disturb_cnt;
    uint8   defect_plane_map[MAX_LLUN_NR];
}spb_info_t;

typedef struct _logcial_lun_writer_t
{
    paa_t                   paa_ptr[MAX_LLUN_NR];
    uint64                  laa_start;
    uint16                  laa_cnt;
    uint16                  current_logical_lun_id;
    logical_lun_operator_t  logcial_lun_operator;
    lut_t                   *plut;
    void                    *simulator;
}logical_lun_writer_t;

typedef struct _logical_lun_reader_t
{
    paa_t                   paa_ptr[MAX_LLUN_NR];
    uint64                  laa_start;
    uint16                  laa_cnt;
    logical_lun_operator_t  logcial_lun_operator;
    lut_t                   *plut;
    mongoc_gridfs_t         *simulator;
}logical_lun_reader_t;
lut_t *get_lut_ptr(uint32 user_type);
void laa_range_write(logical_lun_writer_t *logical_lun_writer);
void laa_range_read(logical_lun_reader_t *logical_lun_reader);




