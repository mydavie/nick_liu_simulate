#include "../lib_header.h"
#include "../nand_vectors/nand_vectors.h"
#include "../nand_virtualize/logical_lun_operator.h"
#include "nand_allocator.h"

typedef uint16 spb_id_t ;

lut_t user_lut;
lut_t sys0_lut;
lut_t sys1_lut;
paa_t l2p_table[100];

spb_info_t gspb_info[MAX_BLOCK_PER_PLANE];

spb_info_t* get_spb_info(uint32 spb_id)
{
	return &gspb_info[spb_id];
}

uint32 is_bit_set(uint32 map, uint32 bit_idx)
{
	return (map & (1UL << bit_idx));
}

void update_lut_by_logical_lun(lut_t *plut, logical_lun_t *plogical_lun)
{
    paa_t paa;
    uint32 laa_cnt      = plogical_lun->laa_cnt;
    uint64 laa_start    = plogical_lun->laa_start;
    paa.logical_lun_id = plogical_lun->logical_lun_id;
    for (uint32 i = 0; i < MAX_PLANE_PER_LUN; i++) {
       if (plogical_lun->au_param.range.au_cnt[i] == 0) {
           continue;
       }
       for (uint32 j = 0; j < plogical_lun->au_param.range.au_cnt[i]; j++) {
           paa.au_ptr_of_lun = plogical_lun->au_param.range.au_start[i]+j;
           plut->paa[laa_start++] = paa;
           laa_cnt--;
       }
    }
    assert(laa_cnt == 0);
}

void update_lut_by_logical_lun_operator(lut_t *plut, logical_lun_operator_t *plogcial_lun_operator)
{
    logical_lun_t *plogical_lun = plogcial_lun_operator->list;

    for (uint32 i = 0; i < plogcial_lun_operator->cnt; i++) {
        update_lut_by_logical_lun(plut, plogical_lun);
        plogical_lun = (logical_lun_t *)plogical_lun->next;
    }
}
void insert_logical_lun_to_operator(logical_lun_operator_t *plogcial_lun_operator,  logical_lun_t *plogical_lun_insert)
{
    if (plogcial_lun_operator->cnt == 0) {
        plogcial_lun_operator->list = plogical_lun_insert;
    }
    else {
        logical_lun_t *plogical_lun = plogcial_lun_operator->list;
        for (uint32 i = 0; i < plogcial_lun_operator->cnt - 1; i++) {
           plogical_lun = (logical_lun_t *)plogical_lun->next;
        }
        plogical_lun->next = (pool_node_t*)plogical_lun_insert;
    }
    plogical_lun_insert->simulator = plogcial_lun_operator->simulator;
    plogcial_lun_operator->cnt++;
}

//if the find cost time, then will be managed by like RB tree.
logical_lun_t * find_logical_lun_from_operator(logical_lun_operator_t *plogcial_lun_operator, uint32 logical_lun_id)
{
    logical_lun_t *plogical_lun = plogcial_lun_operator->list;
    if (plogical_lun == NULL) {
      assert(plogcial_lun_operator->cnt);
      return plogical_lun;
    }
    else {
      for (uint32 i = 0; i < plogcial_lun_operator->cnt; i++) {
         if (plogical_lun->logical_lun_id == logical_lun_id) {
             break;
         }
         plogical_lun = (logical_lun_t *)plogical_lun->next;
      }
    }
    return plogical_lun;
}

logical_lun_t *nand_spb_allocate(paa_t *paa, uint32 want_nr, uint32 *got_nr)
{
    nand_info_t *pnand_info         = get_nand_info();
    spb_info_t* pspb_info           = get_spb_info(paa->spb_id);
    uint32 result_nr;
    uint32 au_nr_per_page_width     = pnand_info->au_nr_per_page_width;
    uint32 au_nr_per_plane_width    = (pspb_info->pb_type == SLC_OTF_TYPE) ? au_nr_per_page_width : (au_nr_per_page_width * pnand_info->bits_nr_per_cell);
    uint32 au_nr_per_lun_width      = au_nr_per_plane_width * pnand_info->plane_nr_per_lun;
    uint32 plane_ptr_of_lun_width   = paa->au_ptr_of_lun / au_nr_per_plane_width;
    logical_lun_t* plogical_lun     = logical_lun_allocate(1, &result_nr);
    //for slc block, the page is the minimal program unit,and tlc block, the word line is minimal program unit.
    assert(paa->au_ptr_of_lun % au_nr_per_plane_width == 0);
    assert(want_nr % au_nr_per_plane_width == 0);
    plogical_lun->llun_nand_type = pspb_info->pb_type;
    plogical_lun->au_param.range.au_start[plane_ptr_of_lun_width] = paa->au_ptr_of_lun;
    do {
        if (is_bit_set(pspb_info->defect_plane_map[plane_ptr_of_lun_width], plane_ptr_of_lun_width))
        {
            plogical_lun->au_param.range.au_start[plane_ptr_of_lun_width]   = 0;
            plogical_lun->au_param.range.au_cnt[plane_ptr_of_lun_width]     = 0;
            paa->au_ptr_of_lun += au_nr_per_lun_width;
        }
        else {
            plogical_lun->au_param.range.au_start[plane_ptr_of_lun_width]   = paa->au_ptr_of_lun;
            plogical_lun->au_param.range.au_cnt[plane_ptr_of_lun_width]     = au_nr_per_plane_width;
            paa->au_ptr_of_lun += plogical_lun->au_param.range.au_cnt[plane_ptr_of_lun_width];
        }
        want_nr -= plogical_lun->au_param.range.au_cnt[plane_ptr_of_lun_width];
        *got_nr  += plogical_lun->au_param.range.au_cnt[plane_ptr_of_lun_width];
        plane_ptr_of_lun_width++;
    } while ((plane_ptr_of_lun_width < MAX_PLANE_PER_LUN) && want_nr);

    return plogical_lun;
}

void laa_range_write(logical_lun_writer_t *logical_lun_writer)
{
    uint32 got_nr;
    uint64 laa_start    = logical_lun_writer->laa_start;
    uint32 want_nr      = logical_lun_writer->laa_cnt;
    uint32 cur_llun_id  = logical_lun_writer->current_logical_lun_id;
    logical_lun_operator_t *plogical_lun_operator = &logical_lun_writer->logcial_lun_operator;

    logical_lun_writer->logcial_lun_operator.op_type = LOGICAL_LUN_OP_TYPE_PROGRAM;
    //laa range translate into logical lun list, and translate to different none busy lun to best the performance later.
    do {
        got_nr = 0;
        logical_lun_t *plogical_lun = nand_spb_allocate(&logical_lun_writer->paa_ptr[cur_llun_id], want_nr, &got_nr);
        plogical_lun->laa_start     = laa_start;
        plogical_lun->laa_cnt       = got_nr;
        insert_logical_lun_to_operator(plogical_lun_operator, plogical_lun);
        //the following is sequential allocate. and the random allocate will find the idle lun to serve the allocator.
        cur_llun_id++;
        if (cur_llun_id == MAX_LLUN_NR) {
            cur_llun_id = 0;
        }
        want_nr     -= got_nr;
        laa_start   += got_nr;
    } while (want_nr);
    assert(laa_start == logical_lun_writer->laa_start + logical_lun_writer->laa_cnt);
    logical_lun_writer->current_logical_lun_id = cur_llun_id;

    //submit_logical_lun_operator(plogical_lun_operator);
    update_lut_by_logical_lun_operator(logical_lun_writer->plut, plogical_lun_operator);
    logical_lun_release(plogical_lun_operator->list, plogical_lun_operator->cnt);
}

/*any range laa, it should be split into different logical lun, either sequential or scatter.
 * the destination of paa maybe scattered in different plane of different logical lun, parse more laa to the same
 * logical lun and no plane conflict, this will make nand vector do decision to do multiple plane read. but parse more laa will
 * cost more resource. and allocated resource fail case will handle later.
 */
#define UPDATE_TO_CURRENT_AUS_SLOT  0
#define UPDATE_TO_NEXT_AUS_SLOT     1
#define UPDATE_TO_NEW_LOGICAL_LUN   2

void record_paa_to_logical_lun(logical_lun_operator_t *plogical_lun_operator, logical_lun_t* plogical_lun, paa_t paa, uint32 type)
{
    uint32 result_nr;
    switch (type)
    {
        case UPDATE_TO_CURRENT_AUS_SLOT:
            plogical_lun->au_param.range.au_cnt[plogical_lun->au_param.range.range_cnt]++;
            break;
        case UPDATE_TO_NEXT_AUS_SLOT:
            plogical_lun->au_param.range.range_cnt++;
            plogical_lun->au_param.range.au_start[plogical_lun->au_param.range.range_cnt] = paa.au_ptr_of_lun;
            plogical_lun->au_param.range.au_cnt[plogical_lun->au_param.range.range_cnt]++;
            break;
        case UPDATE_TO_NEW_LOGICAL_LUN:
            plogical_lun = find_logical_lun_from_operator(plogical_lun_operator, paa.logical_lun_id);
            if (plogical_lun == NULL) {
              plogical_lun = logical_lun_allocate(1, &result_nr);
              insert_logical_lun_to_operator(plogical_lun_operator, plogical_lun);
              assert(result_nr == 1);
            }
            plogical_lun->au_param.range.au_start[plogical_lun->au_param.range.range_cnt] = paa.au_ptr_of_lun;
            plogical_lun->au_param.range.au_cnt[plogical_lun->au_param.range.range_cnt]++;
            plogical_lun->au_param.range.range_cnt++;
            break;
        default:
            break;
    }
}

void laa_range_read(logical_lun_reader_t *logical_lun_reader){
    uint64 laa                  =  logical_lun_reader->laa_start;
    paa_t paa                   = logical_lun_reader->plut->paa[laa];;
    paa_t cur_paa;
    logical_lun_operator_t logical_lun_operator;
    logical_lun_t logical_lun;

    //the paa do not placed into it's dedicated plane position, due to sequential laa maybe translated to conflict paa.
    record_paa_to_logical_lun(&logical_lun_operator, &logical_lun, paa, UPDATE_TO_NEW_LOGICAL_LUN);
    for (uint32 i = 0; i < logical_lun_reader->laa_cnt; i++) {
        cur_paa = logical_lun_reader->plut->paa[laa + i];
        if (cur_paa.logical_lun_id == paa.logical_lun_id) {
            //if the next paa is in the same logical lun, then reuse the same logical lun context.
            if (cur_paa.au_ptr_of_lun == paa.au_ptr_of_lun + 1) {
                //the sequential laa to get sequential paa
                record_paa_to_logical_lun(&logical_lun_operator, &logical_lun, paa, UPDATE_TO_CURRENT_AUS_SLOT);
            }
            else {
                //place the non-sequential paa to another slot, it maybe in same plane or another plane of the same lun
                record_paa_to_logical_lun(&logical_lun_operator, &logical_lun, paa, UPDATE_TO_NEXT_AUS_SLOT);
            }
        }
        else {
            //find previous or allocate another new logical lun context to serve the scattered paa.
            record_paa_to_logical_lun(&logical_lun_operator, &logical_lun, paa, UPDATE_TO_NEW_LOGICAL_LUN);
        }
    }
}
lut_t *get_lut_ptr(uint32 user_type)
{
    return &user_lut;
}
void nand_allocator_init(void)
{
    logical_lun_reader_t logical_lun_reader;
    logical_lun_reader.plut = &user_lut;

}
//
//logical_lun_t *nand_spb_sequential_allocate_logical_lun(spb_ptr_t *spb_ptr, uint32 want_au_nr, uint32 got_au_nr)
//{
//	//1.check defect map
//	//2.check allocate method, sequential or random  (this will find the idle lun)
//    //3.return the logical lun list for this allocate,based on want_au_nr and got_au_nr
//	nand_info_t *pnand_info 		= get_nand_info();
//	spb_info_t* pspb_info 			= get_spb_info(spb_ptr->spb_id);
//	uint32 lun_ptr_of_spb			= spb_ptr->lun_ptr_of_spb;
//	uint32 au_nr_per_page   		= pnand_info->au_nr;
//	uint32 wordline_idex 			= spb_ptr->wordline_idx[lun_ptr_of_spb];
//	uint32 au_ptr_of_lun    		= spb_ptr->au_of_lun_ptr[lun_ptr_of_spb];
//	uint32 au_nr_per_plane          = (pspb_info->pb_type == SLC_OTF_TYPE) ? au_nr_per_page : (au_nr_per_page * pnand_info->cell_bits_nr);
//	uint32 au_nr_bits_of_plane    	= __builtin_ctz(au_nr_per_plane);
//	uint32 plane_nr_per_lun			= pnand_info->plane_nr
//	assert(au_ptr_of_lun & (au_nr_per_page - 1));
//	if (want_au_nr < au_nr_per_plane) {
//		want_au_nr = au_nr_per_plane;
//	}
//	do {
//			uint32 plane_ptr_of_lun	= (au_ptr_of_lun / au_nr_per_plane) & (plane_nr_per_lun - 1);
//			uint32 au_of_plane_idx	= au_ptr_of_lun % au_nr_per_plane;
//			uint32 result_nr;
//			logical_lun_t* plogical_lun = logical_lun_allcoate(1, &result_nr);
//			assert(result_nr == 1);
//			plogical_lun->au_param.range.au_start = au_ptr_of_lun;
//			plogical_lun->au_param.range.au_cnt   = 0;
//			do {
//				//last plane of lun had been allocated, then jump to next lun
//				if (plane_ptr_of_lun == pnand_info->plane_nr) {
//					lun_ptr_of_spb++;
//					break;
//				}
//				//check the plane defect or not
//				if (is_bit_set(pspb_info->defect_plane_map[lun_ptr_of_spb], plane_ptr_of_lun))
//				{
//					if (plogical_lun->au_param.range.au_start != au_ptr_of_lun) {
//						plogical_lun = logical_lun_allocate(1, &result_nr);
//						assert(result_nr == 1);
//					}
//					au_ptr_of_lun += au_nr_per_plane;
//					plogical_lun->au_param.range.au_start = au_ptr_of_lun;
//					plogical_lun->au_param.range.au_cnt   = 0;
//				}
//				else {
//					plogical_lun->au_param.range.au_cnt += au_nr_per_plane;
//				}
//				want_au_nr -= au_nr_per_plane;
//
//			} while (want_au_nr && (plane_ptr_of_lun++ <= pnand_info->plane_nr));
//
//			if (plane_idx == pnand_info->plane_nr - 1) {
//
//			}
//			else {
//
//			}
//	} while (want_au_nr);
//
//
//}
//


