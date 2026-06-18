/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_PAGE_EXT_H
#define __LINUX_PAGE_EXT_H

#include <linux/types.h>
#include <linux/stacktrace.h>
#include <linux/stackdepot.h>
#include <linux/list.h>

struct pglist_data;

struct page_ext_operations {
	size_t offset;
	size_t size;
	bool (*need)(void);
	void (*init)(void);
};

#ifdef CONFIG_PAGE_EXTENSION

enum page_ext_flags {
	PAGE_EXT_OWNER,
	PAGE_EXT_OWNER_ALLOCATED,
#if defined(CONFIG_PAGE_IDLE_FLAG) && !defined(CONFIG_64BIT)
	PAGE_EXT_YOUNG,
	PAGE_EXT_IDLE,
#endif
};

/*
 * Page Extension can be considered as an extended mem_map.
 * A page_ext page is associated with every page descriptor. The
 * page_ext helps us add more information about the page.
 * All page_ext are allocated at boot or memory hotplug event,
 * then the page_ext for pfn always exists.
 */
struct page_ext {
	unsigned long flags;

	/* UTS: CXL hotness / promotion tracking */
	u64 uts_cxl_slot_id;
	u8  uts_in_cxl_promote_list;
	u8  uts_in_dram_demote_list;

	/* UTS: DRAM aging / demotion tracking */
	u8  uts_dram_track_state;
	struct list_head uts_dram_track_link;
	unsigned long uts_dram_tracked_pfn;
	u64 uts_dram_slot_id;

	/* UTS: promotion/demotion sampling */
	u8 uts_promote_sampled;
	u8 uts_promote_sample_hit_hot;
	u8 uts_demote_sampled;
	u8 uts_demote_sample_hit_promote;
};

extern unsigned long page_ext_size;
extern void pgdat_page_ext_init(struct pglist_data *pgdat);

#ifdef CONFIG_SPARSEMEM
static inline void page_ext_init_flatmem(void)
{
}

extern void page_ext_init(void);

static inline void page_ext_init_flatmem_late(void)
{
}
#else
extern void page_ext_init_flatmem(void);
extern void page_ext_init_flatmem_late(void);

static inline void page_ext_init(void)
{
}
#endif

struct page_ext *lookup_page_ext(const struct page *page);

static inline struct page_ext *page_ext_next(struct page_ext *curr)
{
	void *next = curr;

	next += page_ext_size;
	return next;
}

#else /* !CONFIG_PAGE_EXTENSION */

struct page_ext;

static inline void pgdat_page_ext_init(struct pglist_data *pgdat)
{
}

static inline struct page_ext *lookup_page_ext(const struct page *page)
{
	return NULL;
}

static inline void page_ext_init(void)
{
}

static inline void page_ext_init_flatmem_late(void)
{
}

static inline void page_ext_init_flatmem(void)
{
}

#endif /* CONFIG_PAGE_EXTENSION */

/* =========================
 * UTS page_ext helpers
 * ========================= */

#define UTS_DRAM_NONE              0
#define UTS_DRAM_TRACKED           1
#define UTS_DRAM_DEMOTE_CANDIDATE  2

#ifdef CONFIG_PAGE_EXTENSION

static __always_inline u64 uts_page_cxl_slot_id_read(struct page *page)
{
	struct page_ext *pe = lookup_page_ext(page);

	if (!pe)
		return 0;

	return READ_ONCE(pe->uts_cxl_slot_id);
}

static __always_inline void uts_page_cxl_slot_id_write(struct page *page, u64 v)
{
	struct page_ext *pe = lookup_page_ext(page);

	if (!pe)
		return;

	WRITE_ONCE(pe->uts_cxl_slot_id, v);
}

static __always_inline bool uts_page_in_cxl_promote_list(struct page *page)
{
	struct page_ext *pe = lookup_page_ext(page);

	if (!pe)
		return false;

	return READ_ONCE(pe->uts_in_cxl_promote_list);
}

static __always_inline void uts_page_set_in_cxl_promote_list(struct page *page,
							     bool v)
{
	struct page_ext *pe = lookup_page_ext(page);

	if (!pe)
		return;

	WRITE_ONCE(pe->uts_in_cxl_promote_list, v);
}

static __always_inline bool uts_page_in_dram_demote_list(struct page *page)
{
	struct page_ext *pe = lookup_page_ext(page);

	if (!pe)
		return false;

	return READ_ONCE(pe->uts_in_dram_demote_list);
}

static __always_inline void uts_page_set_in_dram_demote_list(struct page *page,
							    bool v)
{
	struct page_ext *pe = lookup_page_ext(page);

	if (!pe)
		return;

	WRITE_ONCE(pe->uts_in_dram_demote_list, v);
}

static __always_inline u8 uts_page_dram_track_state_read(struct page *page)
{
	struct page_ext *pe = lookup_page_ext(page);

	if (!pe)
		return UTS_DRAM_NONE;

	return READ_ONCE(pe->uts_dram_track_state);
}

static __always_inline void uts_page_dram_track_state_write(struct page *page,
							    u8 v)
{
	struct page_ext *pe = lookup_page_ext(page);

	if (!pe)
		return;

	WRITE_ONCE(pe->uts_dram_track_state, v);
}

static __always_inline struct list_head *uts_page_dram_track_link(struct page *page)
{
	struct page_ext *pe = lookup_page_ext(page);

	if (!pe)
		return NULL;

	return &pe->uts_dram_track_link;
}

static __always_inline unsigned long uts_page_dram_tracked_pfn_read(struct page *page)
{
	struct page_ext *pe = lookup_page_ext(page);

	if (!pe)
		return 0;

	return READ_ONCE(pe->uts_dram_tracked_pfn);
}

static __always_inline void uts_page_dram_tracked_pfn_write(struct page *page,
							    unsigned long pfn)
{
	struct page_ext *pe = lookup_page_ext(page);

	if (!pe)
		return;

	WRITE_ONCE(pe->uts_dram_tracked_pfn, pfn);
}

static __always_inline u64 uts_page_dram_slot_id_read(struct page *page)
{
	struct page_ext *pe = lookup_page_ext(page);

	if (!pe)
		return 0;

	return READ_ONCE(pe->uts_dram_slot_id);
}

static __always_inline void uts_page_dram_slot_id_write(struct page *page, u64 v)
{
	struct page_ext *pe = lookup_page_ext(page);

	if (!pe)
		return;

	WRITE_ONCE(pe->uts_dram_slot_id, v);
}

static __always_inline bool uts_page_dram_tracked(struct page *page)
{
	return uts_page_dram_track_state_read(page) != UTS_DRAM_NONE;
}

/*
 * Caller must hold the same lock that protects the DRAM tracking lists.
 */
static __always_inline void uts_page_dram_unlink_if_tracked(struct page *page)
{
	struct page_ext *pe = lookup_page_ext(page);

	if (!pe)
		return;

	if (READ_ONCE(pe->uts_dram_track_state) == UTS_DRAM_NONE)
		return;

	list_del_init(&pe->uts_dram_track_link);
	WRITE_ONCE(pe->uts_dram_track_state, UTS_DRAM_NONE);
}

static __always_inline bool uts_page_promote_sampled(struct page *page)
{
	struct page_ext *pe = lookup_page_ext(page);

	if (!pe)
		return false;

	return READ_ONCE(pe->uts_promote_sampled);
}

static __always_inline void uts_page_set_promote_sampled(struct page *page,
							 bool v)
{
	struct page_ext *pe = lookup_page_ext(page);

	if (!pe)
		return;

	WRITE_ONCE(pe->uts_promote_sampled, v);
}

static __always_inline bool uts_page_promote_sample_hit_hot(struct page *page)
{
	struct page_ext *pe = lookup_page_ext(page);

	if (!pe)
		return false;

	return READ_ONCE(pe->uts_promote_sample_hit_hot);
}

static __always_inline void uts_page_set_promote_sample_hit_hot(struct page *page,
								bool v)
{
	struct page_ext *pe = lookup_page_ext(page);

	if (!pe)
		return;

	WRITE_ONCE(pe->uts_promote_sample_hit_hot, v);
}

static __always_inline bool uts_page_demote_sampled(struct page *page)
{
	struct page_ext *pe = lookup_page_ext(page);

	if (!pe)
		return false;

	return READ_ONCE(pe->uts_demote_sampled);
}

static __always_inline void uts_page_set_demote_sampled(struct page *page,
							bool v)
{
	struct page_ext *pe = lookup_page_ext(page);

	if (!pe)
		return;

	WRITE_ONCE(pe->uts_demote_sampled, v);
}

static __always_inline bool uts_page_demote_sample_hit_promote(struct page *page)
{
	struct page_ext *pe = lookup_page_ext(page);

	if (!pe)
		return false;

	return READ_ONCE(pe->uts_demote_sample_hit_promote);
}

static __always_inline void uts_page_set_demote_sample_hit_promote(struct page *page,
								   bool v)
{
	struct page_ext *pe = lookup_page_ext(page);

	if (!pe)
		return;

	WRITE_ONCE(pe->uts_demote_sample_hit_promote, v);
}

#else /* !CONFIG_PAGE_EXTENSION */

static __always_inline u64 uts_page_cxl_slot_id_read(struct page *page)
{
	return 0;
}

static __always_inline void uts_page_cxl_slot_id_write(struct page *page, u64 v)
{
}

static __always_inline bool uts_page_in_cxl_promote_list(struct page *page)
{
	return false;
}

static __always_inline void uts_page_set_in_cxl_promote_list(struct page *page,
							     bool v)
{
}

static __always_inline bool uts_page_in_dram_demote_list(struct page *page)
{
	return false;
}

static __always_inline void uts_page_set_in_dram_demote_list(struct page *page,
							    bool v)
{
}

static __always_inline u8 uts_page_dram_track_state_read(struct page *page)
{
	return UTS_DRAM_NONE;
}

static __always_inline void uts_page_dram_track_state_write(struct page *page,
							    u8 v)
{
}

static __always_inline struct list_head *uts_page_dram_track_link(struct page *page)
{
	return NULL;
}

static __always_inline unsigned long uts_page_dram_tracked_pfn_read(struct page *page)
{
	return 0;
}

static __always_inline void uts_page_dram_tracked_pfn_write(struct page *page,
							    unsigned long pfn)
{
}

static __always_inline u64 uts_page_dram_slot_id_read(struct page *page)
{
	return 0;
}

static __always_inline void uts_page_dram_slot_id_write(struct page *page, u64 v)
{
}

static __always_inline bool uts_page_dram_tracked(struct page *page)
{
	return false;
}

static __always_inline void uts_page_dram_unlink_if_tracked(struct page *page)
{
}

static __always_inline bool uts_page_promote_sampled(struct page *page)
{
	return false;
}

static __always_inline void uts_page_set_promote_sampled(struct page *page,
							 bool v)
{
}

static __always_inline bool uts_page_promote_sample_hit_hot(struct page *page)
{
	return false;
}

static __always_inline void uts_page_set_promote_sample_hit_hot(struct page *page,
								bool v)
{
}

static __always_inline bool uts_page_demote_sampled(struct page *page)
{
	return false;
}

static __always_inline void uts_page_set_demote_sampled(struct page *page,
							bool v)
{
}

static __always_inline bool uts_page_demote_sample_hit_promote(struct page *page)
{
	return false;
}

static __always_inline void uts_page_set_demote_sample_hit_promote(struct page *page,
								   bool v)
{
}

#endif /* CONFIG_PAGE_EXTENSION */

#endif /* __LINUX_PAGE_EXT_H */