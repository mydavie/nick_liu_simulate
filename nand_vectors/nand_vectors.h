#pragma once
//#include "../utility/types.h"
//#include "../utility/pool_mgr.h"
//#include "../utility/memory_mgr.h"
//#include <mongoc/mongoc.h>
#define KB_BITS                     4
#define KB                          (1 << KB_BITS)
//NAND cell type
#define SLC_OTF_TYPE    			0
#define NATIVE_TYPE     			1
//NAND physical layout bits limitation macro definition
#define MAX_AU_BITS                 (2 + KB_BITS)
#define MAX_CELL_BITS               2  // 4bits per cell
#define MAX_PAGE_SIZE_BITS          (4 + KB_BITS)   //FW support max au offset per physical page
#define MAX_AU_PER_PAGE_BITS        (MAX_PAGE_SIZE_BITS - MAX_AU_BITS)
#define MAX_CH_BITS                 3   //FW support max physical CHANNEL number
#define MAX_CE_PER_CH_BITS          2   //FW support max physical CE number
#define MAX_LUN_PER_CE_BITS         2   //FW support max physical LUN number per physical ce
#define MAX_PLANE_PER_LUN_BITS      2   //FW support max physical PLANE number per physical lun
#define MAX_BLOCK_PER_PLANE_BITS    10  //FW support max physical BLOCK number per physical plane
#define MAX_PAGE_PER_BLOCK_BITS     14  //FW support max physical PAGE number per physical block
#define MAX_PLUN_NR_BITS            (MAX_CH_BITS + MAX_CE_PER_CH_BITS + MAX_LUN_PER_CE_BITS)
#define MAX_LLUN_NR_BITS            MAX_PLUN_NR_BITS
#define MAX_NAND_LAYOUT_SIZE_BITS   (MAX_PLUN_NR_BITS + MAX_AU_BITS)
#define MAX_PLANE_PER_PARALEL_BITS	(MAX_CH_BITS + MAX_CE_PER_CH_BITS + MAX_LUN_PER_CE_BITS + MAX_PLANE_PER_LUN_BITS)
#define MAX_AU_PER_PARALEL_BITS     (MAX_AU_PER_PAGE_BITS + MAX_CELL_BITS + MAX_PLANE_PER_PARALEL_BITS)
#define MAX_AU_PER_LUN_BITS         (MAX_PLANE_PER_PARALEL_BITS + MAX_PAGE_PER_BLOCK_BITS)

#define PAGE_SIZE                   (1UL << MAX_PAGE_SIZE_BITS)
#define AU_SIZE                     (1UL << MAX_AU_BITS)
#define PAGE_PER_PLANE              (1UL << MAX_CELL_BITS)
#define AU_NR_PER_PAGE              (1UL << MAX_AU_PER_PAGE_BITS)
#define MAX_CHANNEL_NR              (1UL << MAX_CH_BITS)
#define MAX_CE_PER_CHANNEL          (1UL << MAX_CE_PER_CH_BITS)
#define MAX_LUN_PER_CE              (1UL << MAX_LUN_PER_CE_BITS)
#define MAX_PLANE_PER_LUN           (1UL << MAX_PLANE_PER_LUN_BITS)
#define MAX_BLOCK_PER_PLANE         (1UL << MAX_BLOCK_PER_PLANE_BITS)
#define MAX_PAGE_PER_BLOCK          (1UL << MAX_PAGE_PER_BLOCK_BITS)
#define MAX_PLUN_NR                 (1UL << MAX_PLUN_NR_BITS)
#define MAX_LLUN_NR                 (1UL << MAX_LLUN_NR_BITS)
#define MAX_AU_PER_PARALEL          (1UL << MAX_AU_PER_PARALEL_BITS)
#define MAX_AU_PER_LUN              (1UL << MAX_AU_PER_LUN_BITS)

//the NAND layout physical address that used by ASIC, and transfered from PAA
#define NAND_OPERATOR_POOL_NR   MAX_LLUN_NR
#define NAND_VECTOR_POOL_NR     (NAND_OPERATOR_POOL_NR * (MAX_PLANE_PER_LUN * AU_NR_PER_PAGE))
/*llun is the abbreviation of logical lun, plun is the abbreviation of physical lun.
 * llun is used by FTL(flash tanslation layer, manage the NAND), just a sequence ID.
 * plun is used by FCL(flash controller layer, operate NAND interface).
 * llun will be translated to plun by llun_to_plun table, this will fast the mapping process of get the dedicated NAND position.
 * one translation as following, and different map sequence will generate different FCL parallel performance:
         plun     -->  llun

     ch0 ce0 lun0 -->  llun0
     ch1 ce0 lun0 -->  llun1
     ch2 ce0 lun0 -->  llun2
        ................
*/
typedef union _physical_lun_t
{
    struct {
        uint32 ch       : MAX_CH_BITS;
        uint32 ce       : MAX_CE_PER_CH_BITS;
        uint32 lun      : MAX_LUN_PER_CE_BITS;
    }field;
    uint32 value;
}physical_lun_t;

typedef union _physical_lun_ptr_t
{
    struct {
    uint32 plane            : MAX_PLANE_PER_LUN_BITS;
    uint32 block            : MAX_BLOCK_PER_PLANE_BITS;
    uint32 page             : MAX_PAGE_PER_BLOCK_BITS;
    uint32 au_off           : MAX_AU_PER_PAGE_BITS;
    uint32 rsvd             : (32 - MAX_PLANE_PER_LUN_BITS - MAX_BLOCK_PER_PLANE_BITS - MAX_PAGE_PER_BLOCK_BITS - MAX_AU_PER_PAGE_BITS);
    }field;
    uint32 value;
}physical_lun_ptr_t;

typedef struct _vector_t
{
    physical_lun_ptr_t  phy_lun_ptr;
    physical_lun_t      phy_lun;
}vector_t;

typedef struct _nand_vector_t
{
    pool_node_t         *next;
    void                *logcial_lun;
    vector_t            vector;
    uint16              au_cnt;
    void                *simulator;
    memory_node_t       *buf_node;
}nand_vector_t;
typedef struct _nand_vector_operator_t
{
    nand_vector_t*  list_start;
    nand_vector_t   start;
    uint16          cnt;
}nand_vector_operator_t;


//NAND operator type
#define OP_TYPE_READ_NORMAL     0
#define OP_TYPE_PROGRAM_NORMAL  1

//typedef struct _nand_operator_t
//{
//    pool_node_t             *next;
//    uint8                   *buf;                                   //the DRAM address that save user data
//    uint16                  size;                                   //the user data size
//    uint16                  op_cnt;
//    uint16                  op_type;
//    uint16                  outstanding_vector_cnt;
//    logcial_lun_operator_t  logical_lun_operator;
//    nand_vector_operator_t  vector_operator;
//    void                    *simulator_ptr;
//}nand_operator_t;

typedef struct _nand_info_t
{
    uint64 ch_nr                : MAX_CH_BITS + 1;
    uint64 ce_nr                : MAX_CE_PER_CH_BITS + 1;
    uint64 lun_nr               : MAX_LUN_PER_CE_BITS + 1;
    uint64 plane_nr_per_lun     : MAX_PLANE_PER_LUN_BITS + 1;
    uint64 block_nr_per_plane   : MAX_BLOCK_PER_PLANE_BITS + 1;
    uint64 page_nr_per_block    : MAX_PAGE_PER_BLOCK_BITS + 1;
    uint64 au_nr_per_page_width : MAX_AU_PER_PAGE_BITS + 1;
    uint64 bits_nr_per_cell     : MAX_CELL_BITS + 1;
    uint64 rsvd                 : (64 - MAX_CH_BITS - MAX_CE_PER_CH_BITS - MAX_LUN_PER_CE_BITS - MAX_PLANE_PER_LUN_BITS \
    - MAX_BLOCK_PER_PLANE_BITS - MAX_PAGE_PER_BLOCK_BITS - MAX_AU_PER_PAGE_BITS - MAX_CELL_BITS- 8);
    uint32 au_nr_per_slc_lun;
    uint32 au_nr_per_slc_plane;
    uint8  au_nr_per_slc_plane_width;
    uint8  au_nr_per_slc_lun_width;
    uint8  au_nr_per_xlc_plane_width;
    uint8  au_nr_per_xlc_lun_width;
    uint32 au_nr_per_xlc_lun;
    uint32 au_nr_per_xlc_plane;
}nand_info_t;

typedef struct _nand_mgr_t
{
    pool_mgr_t vector_pool_mgr;

}nand_mgr_t;

void nand_init_onetime(void);
uint32 read_nand_vector(nand_vector_t* pnand_vector);
uint32 write_nand_vector(nand_vector_t* pnand_vector);
nand_vector_t* nand_allcoate_vector(uint32 want_nr, uint32 *result_nr);
uint32 nand_release_vectors(nand_vector_t* start, uint32 vector_cnt);
nand_info_t * get_nand_info(void);

