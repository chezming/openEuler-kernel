/*
 * Copyright (c) 2015 Linaro Ltd.
 * Copyright (c) 2015 Hisilicon Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef _HISI_SAS_H_
#define _HISI_SAS_H_

#include <linux/acpi.h>
#include <linux/clk.h>
#include <linux/debugfs.h>
#include <linux/dmapool.h>
#include <linux/iopoll.h>
#include <linux/lcm.h>
#include <linux/libata.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/property.h>
#include <linux/regmap.h>
#include <scsi/scsi_common.h>
#include <scsi/sas_ata.h>
#include <scsi/libsas.h>

#define HISI_SAS_MAX_PHYS	9
#define HISI_SAS_MAX_QUEUES	32
#define HISI_SAS_QUEUE_SLOTS	4096
#define HISI_SAS_MAX_ITCT_ENTRIES 1024
#define HISI_SAS_MAX_DEVICES HISI_SAS_MAX_ITCT_ENTRIES
#define HISI_SAS_RESET_BIT	0
#define HISI_SAS_REJECT_CMD_BIT	1
#define HISI_SAS_RESERVED_IPTT_CNT  96

#define HISI_SAS_STATUS_BUF_SZ (sizeof(struct hisi_sas_status_buffer))
#define HISI_SAS_COMMAND_TABLE_SZ (sizeof(union hisi_sas_command_table))

#define hisi_sas_status_buf_addr(buf) \
	(buf + offsetof(struct hisi_sas_slot_buf_table, status_buffer))
#define hisi_sas_status_buf_addr_mem(slot) hisi_sas_status_buf_addr(slot->buf)
#define hisi_sas_status_buf_addr_dma(slot) \
	hisi_sas_status_buf_addr(slot->buf_dma)

#define hisi_sas_cmd_hdr_addr(buf) \
	(buf + offsetof(struct hisi_sas_slot_buf_table, command_header))
#define hisi_sas_cmd_hdr_addr_mem(slot) hisi_sas_cmd_hdr_addr(slot->buf)
#define hisi_sas_cmd_hdr_addr_dma(slot) hisi_sas_cmd_hdr_addr(slot->buf_dma)

#define hisi_sas_sge_addr(buf) \
	(buf + offsetof(struct hisi_sas_slot_buf_table, sge_page))
#define hisi_sas_sge_addr_mem(slot) hisi_sas_sge_addr(slot->buf)
#define hisi_sas_sge_addr_dma(slot) hisi_sas_sge_addr(slot->buf_dma)

#define hisi_sas_sge_dif_addr(buf) \
	(buf + offsetof(struct hisi_sas_slot_dif_buf_table, \
			sge_dif_page))
#define hisi_sas_sge_dif_addr_mem(slot) hisi_sas_sge_dif_addr(slot->buf)
#define hisi_sas_sge_dif_addr_dma(slot) hisi_sas_sge_dif_addr(slot->buf_dma)

#define HISI_SAS_MAX_SSP_RESP_SZ (sizeof(struct ssp_frame_hdr) + 1024)
#define HISI_SAS_MAX_SMP_RESP_SZ 1028
#define HISI_SAS_MAX_STP_RESP_SZ 28

#define DEV_IS_EXPANDER(type) \
	((type == SAS_EDGE_EXPANDER_DEVICE) || \
	(type == SAS_FANOUT_EXPANDER_DEVICE))

#define HISI_SAS_SATA_PROTOCOL_NONDATA		0x1
#define HISI_SAS_SATA_PROTOCOL_PIO			0x2
#define HISI_SAS_SATA_PROTOCOL_DMA			0x4
#define HISI_SAS_SATA_PROTOCOL_FPDMA		0x8
#define HISI_SAS_SATA_PROTOCOL_ATAPI		0x10

#define HISI_SAS_DIF_PROT_MASK (SHOST_DIF_TYPE1_PROTECTION | \
				SHOST_DIF_TYPE2_PROTECTION | \
				SHOST_DIF_TYPE3_PROTECTION)

#define HISI_SAS_DIX_PROT_MASK (SHOST_DIX_TYPE1_PROTECTION | \
				SHOST_DIX_TYPE2_PROTECTION | \
				SHOST_DIX_TYPE3_PROTECTION)

#define HISI_SAS_PROT_MASK (HISI_SAS_DIF_PROT_MASK | HISI_SAS_DIX_PROT_MASK)

#define HISI_SAS_WAIT_PHYUP_TIMEOUT	(30 * HZ)
#define CLEAR_ITCT_TIMEOUT	20

struct hisi_hba;

enum {
	PORT_TYPE_SAS = (1U << 1),
	PORT_TYPE_SATA = (1U << 0),
};

enum dev_status {
	HISI_SAS_DEV_INIT,
	HISI_SAS_DEV_NORMAL,
	HISI_SAS_DEV_NCQ_ERR,
};

enum {
	HISI_SAS_INT_ABT_CMD = 0,
	HISI_SAS_INT_ABT_DEV = 1,
};

enum hisi_sas_dev_type {
	HISI_SAS_DEV_TYPE_STP = 0,
	HISI_SAS_DEV_TYPE_SSP,
	HISI_SAS_DEV_TYPE_SATA,
};

enum datapres_field {
	NO_DATA         = 0,
	RESPONSE_DATA   = 1,
	SENSE_DATA      = 2,
};

struct hisi_sas_hw_error {
	u32 irq_msk;
	u32 msk;
	int shift;
	const char *msg;
	int reg;
	const struct hisi_sas_hw_error *sub;
};

struct hisi_sas_rst {
	struct hisi_hba *hisi_hba;
	struct completion *completion;
	struct work_struct work;
	bool done;
};

#define HISI_SAS_RST_WORK_INIT(r, c) \
	{	.hisi_hba = hisi_hba, \
		.completion = &c, \
		.work = __WORK_INITIALIZER(r.work, \
				hisi_sas_sync_rst_work_handler), \
		.done = false, \
		}

#define HISI_SAS_DECLARE_RST_WORK_ON_STACK(r) \
	DECLARE_COMPLETION_ONSTACK(c); \
	DECLARE_WORK(w, hisi_sas_sync_rst_work_handler); \
	struct hisi_sas_rst r = HISI_SAS_RST_WORK_INIT(r, c)

enum hisi_sas_bit_err_type {
	HISI_SAS_ERR_SINGLE_BIT_ECC = 0x0,
	HISI_SAS_ERR_MULTI_BIT_ECC = 0x1,
};

enum hisi_sas_phy_event {
	HISI_PHYE_PHY_UP   = 0U,
	HISI_PHYE_LINK_RESET,
	HISI_PHYES_NUM,
};

struct hisi_sas_phy {
	struct work_struct	works[HISI_PHYES_NUM];
	struct hisi_hba	*hisi_hba;
	struct hisi_sas_port	*port;
	struct asd_sas_phy	sas_phy;
	struct sas_identify	identify;
	struct completion *reset_completion;
	struct timer_list timer;
	spinlock_t lock;
	u64		port_id; /* from hw */
	u64		frame_rcvd_size;
	u8		frame_rcvd[32];
	u8		phy_attached;
	u8		in_reset;
	u8		need_notify;
	u8		reserved;
	u32		phy_type;
	enum sas_linkrate	minimum_linkrate;
	enum sas_linkrate	maximum_linkrate;
	u32		code_error_count;
	int enable;
};

struct hisi_sas_port {
	struct asd_sas_port	sas_port;
	u8	port_attached;
	u8	id; /* from hw */
};

struct hisi_sas_cq {
	struct hisi_hba *hisi_hba;
	struct tasklet_struct tasklet;
	int	rd_point;
	int	id;
};

struct hisi_sas_dq {
	struct hisi_hba *hisi_hba;
	struct list_head list;
	spinlock_t lock;
	int	wr_point;
	int	id;
};

struct hisi_sas_device {
	struct hisi_hba		*hisi_hba;
	struct domain_device	*sas_device;
	struct completion *completion;
	struct hisi_sas_dq	*dq;
	struct list_head	list;
	enum sas_device_type	dev_type;
	unsigned int device_id;
	int sata_idx;
	spinlock_t lock;
	enum dev_status dev_status;
};

struct hisi_sas_tmf_task {
	int force_phy;
	int phy_id;
	u8 tmf;
	u16 tag_of_task_to_be_managed;
};

struct hisi_sas_slot {
	struct list_head entry;
	struct list_head delivery;
	struct sas_task *task;
	struct hisi_sas_port	*port;
	u64	n_elem;
	u64	n_elem_dif;
	int	dlvry_queue;
	int	dlvry_queue_slot;
	int	cmplt_queue;
	int	cmplt_queue_slot;
	int	abort;
	int	ready;
	int	device_id;
	void	*cmd_hdr;
	dma_addr_t cmd_hdr_dma;
	struct timer_list internal_abort_timer;
	bool is_internal;
	struct hisi_sas_tmf_task *tmf;
	/* Do not reorder/change members after here */
	void	*buf;
	dma_addr_t buf_dma;
	int	idx;
};

#define HISI_SAS_DEBUGFS_REG(x) {#x, x}

struct hisi_sas_debugfs_reg_lu {
	char *name;
	int off;
};

struct hisi_sas_debugfs_reg {
	const struct hisi_sas_debugfs_reg_lu *lu;
	int count;
	int base_off;
	union {
		u32 (*read_global_reg)(struct hisi_hba *hisi_hba, u32 off);
		u32 (*read_port_reg)(struct hisi_hba *hisi_hba, int port,
				     u32 off);
	};
};

enum {
	HISI_SAS_BIST_LOOPBACK_MODE_DIGITAL = 0,
	HISI_SAS_BIST_LOOPBACK_MODE_SERDES,
	HISI_SAS_BIST_LOOPBACK_MODE_REMOTE,
};

enum {
	HISI_SAS_BIST_CODE_MODE_PRBS7 = 0,
	HISI_SAS_BIST_CODE_MODE_PRBS23,
	HISI_SAS_BIST_CODE_MODE_PRBS31,
	HISI_SAS_BIST_CODE_MODE_JTPAT,
	HISI_SAS_BIST_CODE_MODE_CJTPAT,
	HISI_SAS_BIST_CODE_MODE_SCRAMBED_0,
	HISI_SAS_BIST_CODE_MODE_TRAIN,
	HISI_SAS_BIST_CODE_MODE_TRAIN_DONE,
	HISI_SAS_BIST_CODE_MODE_HFTP,
	HISI_SAS_BIST_CODE_MODE_MFTP,
	HISI_SAS_BIST_CODE_MODE_LFTP,
	HISI_SAS_BIST_CODE_MODE_FIXED_DATA,
};


struct hisi_sas_hw {
	int (*hw_init)(struct hisi_hba *hisi_hba);
	void (*setup_itct)(struct hisi_hba *hisi_hba,
			   struct hisi_sas_device *device);
	int (*slot_index_alloc)(struct hisi_hba *hisi_hba,
			struct domain_device *device);
	struct hisi_sas_device *(*alloc_dev)(struct domain_device *device);
	void (*sl_notify_ssp)(struct hisi_hba *hisi_hba, int phy_no);
	void (*start_delivery)(struct hisi_sas_dq *dq);
	void (*prep_ssp)(struct hisi_hba *hisi_hba,
			struct hisi_sas_slot *slot);
	void (*prep_smp)(struct hisi_hba *hisi_hba,
			struct hisi_sas_slot *slot);
	void (*prep_stp)(struct hisi_hba *hisi_hba,
			struct hisi_sas_slot *slot);
	void (*prep_abort)(struct hisi_hba *hisi_hba,
			   struct hisi_sas_slot *slot,
			   unsigned int device_id,
			   int abort_flag, int tag_to_abort);
	int (*slot_complete)(struct hisi_hba *hisi_hba,
			     struct hisi_sas_slot *slot);
	void (*phys_init)(struct hisi_hba *hisi_hba);
	void (*phy_start)(struct hisi_hba *hisi_hba, int phy_no);
	void (*phy_disable)(struct hisi_hba *hisi_hba, int phy_no);
	void (*phy_hard_reset)(struct hisi_hba *hisi_hba, int phy_no);
	void (*get_events)(struct hisi_hba *hisi_hba, int phy_no);
	void (*phy_set_linkrate)(struct hisi_hba *hisi_hba, int phy_no,
			struct sas_phy_linkrates *linkrates);
	enum sas_linkrate (*phy_get_max_linkrate)(void);
	int (*clear_itct)(struct hisi_hba *hisi_hba,
			    struct hisi_sas_device *dev);
	void (*free_device)(struct hisi_sas_device *sas_dev);
	int (*get_wideport_bitmap)(struct hisi_hba *hisi_hba, int port_id);
	void (*dereg_device)(struct hisi_hba *hisi_hba,
				struct domain_device *device);
	int (*soft_reset)(struct hisi_hba *hisi_hba);
	u32 (*get_phys_state)(struct hisi_hba *hisi_hba);
	int (*write_gpio)(struct hisi_hba *hisi_hba, u8 reg_type,
				u8 reg_index, u8 reg_count, u8 *write_data);
	int (*wait_cmds_complete_timeout)(struct hisi_hba *hisi_hba,
					   int delay_ms, int timeout_ms);
	void (*snapshot_prepare)(struct hisi_hba *hisi_hba);
	void (*snapshot_restore)(struct hisi_hba *hisi_hba);
	const struct cpumask *(*get_managed_irq_aff)(struct hisi_hba
			*hisi_hba, int queue);
	void (*debugfs_work_handler)(struct work_struct *work);
	int max_command_entries;
	int complete_hdr_size;
	struct scsi_host_template *sht;

	const struct hisi_sas_debugfs_reg *debugfs_reg_global;
	const struct hisi_sas_debugfs_reg *debugfs_reg_port;
	int (*set_bist)(struct hisi_hba *hisi_hba, bool enable);
};

struct hisi_hba {
	/* This must be the first element, used by SHOST_TO_SAS_HA */
	struct sas_ha_struct *p;

	struct platform_device *platform_dev;
	struct pci_dev *pci_dev;
	struct device *dev;

	void __iomem *regs;
	void __iomem *sgpio_regs;
	struct regmap *ctrl;
	u32 ctrl_reset_reg;
	u32 ctrl_reset_sts_reg;
	u32 ctrl_clock_ena_reg;
	u32 refclk_frequency_mhz;
	u8 sas_addr[SAS_ADDR_SIZE];

	int n_phy;
	spinlock_t lock;
	struct semaphore sem;

	struct timer_list timer;
	struct workqueue_struct *wq;

	int slot_index_count;
	int last_slot_index;
	int last_dev_id;
	unsigned long *slot_index_tags;
	unsigned long reject_stp_links_msk;

	/* SCSI/SAS glue */
	struct sas_ha_struct sha;
	struct Scsi_Host *shost;

	struct hisi_sas_cq cq[HISI_SAS_MAX_QUEUES];
	struct hisi_sas_dq dq[HISI_SAS_MAX_QUEUES];
	struct hisi_sas_phy phy[HISI_SAS_MAX_PHYS];
	struct hisi_sas_port port[HISI_SAS_MAX_PHYS];

	int	queue_count;

	struct hisi_sas_device	devices[HISI_SAS_MAX_DEVICES];
	struct hisi_sas_cmd_hdr	*cmd_hdr[HISI_SAS_MAX_QUEUES];
	dma_addr_t cmd_hdr_dma[HISI_SAS_MAX_QUEUES];
	void *complete_hdr[HISI_SAS_MAX_QUEUES];
	dma_addr_t complete_hdr_dma[HISI_SAS_MAX_QUEUES];
	struct hisi_sas_initial_fis *initial_fis;
	dma_addr_t initial_fis_dma;
	struct hisi_sas_itct *itct;
	dma_addr_t itct_dma;
	struct hisi_sas_iost *iost;
	dma_addr_t iost_dma;
	struct hisi_sas_breakpoint *breakpoint;
	dma_addr_t breakpoint_dma;
	struct hisi_sas_breakpoint *sata_breakpoint;
	dma_addr_t sata_breakpoint_dma;
	struct hisi_sas_slot	*slot_info;
	unsigned long flags;
	const struct hisi_sas_hw *hw;	/* Low level hw interface */
	unsigned long sata_dev_bitmap[BITS_TO_LONGS(HISI_SAS_MAX_DEVICES)];
	struct work_struct rst_work;
	struct work_struct debugfs_work;
	struct work_struct notify_work;
	u32 phy_state;
	u32 intr_coal_ticks;	/* time of interrupt coalesce, unit:1us */
	u32 intr_coal_count;	/* count of interrupt coalesce */
	/* bist */
	int bist_loopback_linkrate;
	int bist_loopback_code_mode;
	int bist_loopback_phy_id;
	int bist_loopback_mode;
	u32 bist_loopback_cnt;
	int bist_loopback_enable;

	int enable_dix_dif;

	/* debugfs memories */
	void *debugfs_global_reg;
	void *debugfs_port_reg[HISI_SAS_MAX_PHYS];
	void *debugfs_complete_hdr[HISI_SAS_MAX_QUEUES];
	struct hisi_sas_cmd_hdr	*debugfs_cmd_hdr[HISI_SAS_MAX_QUEUES];
	struct hisi_sas_iost *debugfs_iost;
	struct hisi_sas_itct *debugfs_itct;

	struct dentry *debugfs_dir;
	struct dentry *debugfs_dump_dentry;
	struct dentry *debugfs_bist_dentry;

	bool user_ctl_irq;
	unsigned int dq_idx[NR_CPUS];
	int nvecs;
	unsigned int dq_num_per_node;
};

/* Generic HW DMA host memory structures */
/* Delivery queue header */
struct hisi_sas_cmd_hdr {
	/* dw0 */
	__le32 dw0;

	/* dw1 */
	__le32 dw1;

	/* dw2 */
	__le32 dw2;

	/* dw3 */
	__le32 transfer_tags;

	/* dw4 */
	__le32 data_transfer_len;

	/* dw5 */
	__le32 first_burst_num;

	/* dw6 */
	__le32 sg_len;

	/* dw7 */
	__le32 dw7;

	/* dw8-9 */
	__le64 cmd_table_addr;

	/* dw10-11 */
	__le64 sts_buffer_addr;

	/* dw12-13 */
	__le64 prd_table_addr;

	/* dw14-15 */
	__le64 dif_prd_table_addr;
};

struct hisi_sas_itct {
	__le64 qw0;
	__le64 sas_addr;
	__le64 qw2;
	__le64 qw3;
	__le64 qw4_15[12];
};

struct hisi_sas_iost {
	__le64 qw0;
	__le64 qw1;
	__le64 qw2;
	__le64 qw3;
};

struct hisi_sas_err_record {
	u32	data[4];
};

struct hisi_sas_initial_fis {
	struct hisi_sas_err_record err_record;
	struct dev_to_host_fis fis;
	u32 rsvd[3];
};

struct hisi_sas_breakpoint {
	u8	data[128];
};

struct hisi_sas_sata_breakpoint {
	struct hisi_sas_breakpoint tag[32];
};

struct hisi_sas_sge {
	__le64 addr;
	__le32 page_ctrl_0;
	__le32 page_ctrl_1;
	__le32 data_len;
	__le32 data_off;
};

struct hisi_sas_command_table_smp {
	u8 bytes[44];
};

struct hisi_sas_command_table_stp {
	struct	host_to_dev_fis command_fis;
	u8	dummy[12];
	u8	atapi_cdb[ATAPI_CDB_LEN];
};

#define HISI_SAS_SGE_PAGE_CNT (124)
struct hisi_sas_sge_page {
	struct hisi_sas_sge sge[HISI_SAS_SGE_PAGE_CNT];
}  __aligned(16);

#define HISI_SAS_SGE_DIF_PAGE_CNT   HISI_SAS_SGE_PAGE_CNT
struct hisi_sas_sge_dif_page {
	struct hisi_sas_sge sge[HISI_SAS_SGE_DIF_PAGE_CNT];
}  __aligned(16);

struct hisi_sas_command_table_ssp {
	struct ssp_frame_hdr hdr;
	union {
		struct {
			struct ssp_command_iu task;
			u32 prot[7];
		};
		struct ssp_tmf_iu ssp_task;
		struct xfer_rdy_iu xfer_rdy;
		struct ssp_response_iu ssp_res;
	} u;
};

union hisi_sas_command_table {
	struct hisi_sas_command_table_ssp ssp;
	struct hisi_sas_command_table_smp smp;
	struct hisi_sas_command_table_stp stp;
}  __aligned(16);

struct hisi_sas_status_buffer {
	struct hisi_sas_err_record err;
	u8	iu[1024];
}  __aligned(16);

struct hisi_sas_slot_buf_table {
	struct hisi_sas_status_buffer status_buffer;
	union hisi_sas_command_table command_header;
	struct hisi_sas_sge_page sge_page;
};

struct hisi_sas_slot_dif_buf_table {
	struct hisi_sas_slot_buf_table slot_buf;
	struct hisi_sas_sge_dif_page sge_dif_page;
};

extern bool hisi_sas_debugfs_enable;
extern struct dentry *hisi_sas_debugfs_dir;
extern int skip_bus_flag;
extern struct scsi_transport_template *hisi_sas_stt;
extern void hisi_sas_stop_phys(struct hisi_hba *hisi_hba);
extern int hisi_sas_alloc(struct hisi_hba *hisi_hba);
extern void hisi_sas_free(struct hisi_hba *hisi_hba);
extern u8 hisi_sas_get_ata_protocol(struct host_to_dev_fis *fis,
				int direction);
extern struct hisi_sas_port *to_hisi_sas_port(struct asd_sas_port *sas_port);
extern void hisi_sas_sata_done(struct sas_task *task,
			    struct hisi_sas_slot *slot);
extern int hisi_sas_get_fw_info(struct hisi_hba *hisi_hba);
extern int hisi_sas_probe(struct platform_device *pdev,
			  const struct hisi_sas_hw *ops);
extern int hisi_sas_remove(struct platform_device *pdev);

extern int hisi_sas_slave_configure(struct scsi_device *sdev);
extern int hisi_sas_scan_finished(struct Scsi_Host *shost, unsigned long time);
extern void hisi_sas_scan_start(struct Scsi_Host *shost);
extern int hisi_sas_host_reset(struct Scsi_Host *shost, int reset_type);
extern void hisi_sas_phy_enable(struct hisi_hba *hisi_hba, int phy_no,
				int enable);
extern void hisi_sas_phy_down(struct hisi_hba *hisi_hba, int phy_no, int rdy);
extern void hisi_sas_slot_task_free(struct hisi_hba *hisi_hba,
				    struct sas_task *task,
				    struct hisi_sas_slot *slot,
				    bool need_lock);
extern void hisi_sas_init_mem(struct hisi_hba *hisi_hba);
extern void hisi_sas_rst_work_handler(struct work_struct *work);
extern void hisi_sas_sync_rst_work_handler(struct work_struct *work);
extern void hisi_sas_kill_tasklets(struct hisi_hba *hisi_hba);
extern bool hisi_sas_notify_phy_event(struct hisi_sas_phy *phy,
				enum hisi_sas_phy_event event);
extern void hisi_sas_release_tasks(struct hisi_hba *hisi_hba);
extern u8 hisi_sas_get_prog_phy_linkrate_mask(enum sas_linkrate max);
extern void hisi_sas_controller_reset_prepare(struct hisi_hba *hisi_hba);
extern void hisi_sas_controller_reset_done(struct hisi_hba *hisi_hba);
extern void hisi_sas_debugfs_init(struct hisi_hba *hisi_hba);
extern void hisi_sas_debugfs_exit(struct hisi_hba *hisi_hba);
extern void hisi_sas_snapshot_regs(struct hisi_hba *hisi_hba);
extern void hisi_sas_debugfs_work_handler(struct work_struct *work);
#endif
