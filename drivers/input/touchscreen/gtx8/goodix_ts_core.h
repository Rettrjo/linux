/*
 * Goodix Touchscreen Driver
 * Core layer of touchdriver architecture.
 *
 * Copyright (C) 2019 - 2020 Goodix, Inc.
 * Authors: Wang Yafei <wangyafei@goodix.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be a reference
 * to you, when you are integrating the GOODiX's CTP IC into your system,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */
#ifndef _GOODIX_TS_CORE_H_
#define _GOODIX_TS_CORE_H_

#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/unaligned.h>
#include <linux/vmalloc.h>
#include <linux/version.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#ifdef CONFIG_FB
#include <linux/notifier.h>
#include <linux/fb.h>
#endif

#define GOODIX_FLASH_CONFIG_WITH_ISP 1
/* macros definition */
#define GOODIX_CORE_DRIVER_NAME "goodix_ts"
#define GOODIX_PEN_DRIVER_NAME "goodix_ts,pen"
#define GOODIX_DRIVER_VERSION "v1.4.4.0"
#define GOODIX_BUS_RETRY_TIMES 3
#define GOODIX_MAX_TOUCH 10
#define GOODIX_CFG_MAX_SIZE 4096
#define GOODIX_ESD_TICK_WRITE_DATA 0xAA
#define GOODIX_PID_MAX_LEN 8
#define GOODIX_VID_MAX_LEN 8

#define IC_TYPE_NORMANDY 0
#define IC_TYPE_YELLOWSTONE 1

#define GOODIX_TOUCH_EVENT 0x80
#define GOODIX_REQUEST_EVENT 0x40
#define GOODIX_GESTURE_EVENT 0x20
#define GOODIX_HOTKNOT_EVENT 0x10

#define GOODIX_PEN_MAX_PRESSURE 4096
#define GOODIX_MAX_TP_KEY 4
#define GOODIX_MAX_PEN_KEY 2

/*
 * struct goodix_ts_board_data -  board data
 * @avdd_name: name of analoy regulator
 * @reset_gpio: reset gpio number
 * @irq_gpio: interrupt gpio number
 * @irq_flag: irq trigger type
 * @power_on_delay_us: power on delay time (us)
 * @power_off_delay_us: power off delay time (us)
 * @swap_axis: whether swaw x y axis
 * @panel_max_x/y/w/p: resolution and size
 * @panel_max_key: max supported keys
 * @pannel_key_map: key map
 * @fw_name: name of the firmware image
 */
struct goodix_ts_board_data {
	char avdd_name[24];
	unsigned int avdd_load;
	unsigned int reset_gpio;
	unsigned int irq_gpio;
	unsigned int vdd_gpio;
	int irq;
	unsigned int irq_flags;

	unsigned int power_on_delay_us;
	unsigned int power_off_delay_us;

	unsigned int swap_axis;
	unsigned int panel_max_x;
	unsigned int panel_max_y;
	unsigned int panel_max_w; /*major and minor*/
	unsigned int panel_max_p; /*pressure*/
	unsigned int panel_max_key;
	unsigned int panel_key_map[GOODIX_MAX_TP_KEY];
	unsigned int x2x;
	unsigned int y2y;
	unsigned int tp_key_num;
	/*add end*/

	const char *fw_name;
	const char *cfg_bin_name;
	bool esd_default_on;
};

/*
 * struct goodix_ts_cmd - command package
 * @initialized: whether initialized
 * @cmd_reg: command register
 * @length: command length in bytes
 * @cmds: command data
 */
#pragma pack(4)
struct goodix_ts_cmd {
	u32 initialized;
	u32 cmd_reg;
	u32 length;
	u8 cmds[8];
};
#pragma pack()

/* interrupt event type */
enum ts_event_type {
	EVENT_INVALID = 0,
	EVENT_TOUCH = (1 << 0), /* finger touch event */
	EVENT_PEN = (1 << 1), /* pen event */
	EVENT_REQUEST = (1 << 2),
};

/* notifier event */
enum ts_notify_event {
	NOTIFY_SUSPEND,
	NOTIFY_RESUME,
	NOTIFY_ESD_OFF,
	NOTIFY_ESD_ON,
};

enum touch_point_status {
	TS_NONE,
	TS_RELEASE,
	TS_TOUCH,
};
/* coordinate package */
struct goodix_ts_coords {
	int status; /* NONE, RELEASE, TOUCH */
	unsigned int x, y, w, p;
};

struct goodix_pen_coords {
	int status; /* NONE, RELEASE, TOUCH */
	int tool_type; /* BTN_TOOL_RUBBER BTN_TOOL_PEN */
	unsigned int x, y, p;
	signed char tilt_x;
	signed char tilt_y;
};

struct goodix_ts_key {
	int status;
	int code;
};

/* touch event data */
struct goodix_touch_data {
	int touch_num;
	struct goodix_ts_coords coords[GOODIX_MAX_TOUCH];
	struct goodix_ts_key keys[GOODIX_MAX_TP_KEY];
};

struct goodix_pen_data {
	struct goodix_pen_coords coords;
	struct goodix_ts_key keys[GOODIX_MAX_PEN_KEY];
};

/*
 * struct goodix_ts_event - touch event struct
 * @event_type: touch event type, touch data or
 *	request event
 * @event_data: event data
 */
struct goodix_ts_event {
	enum ts_event_type event_type;
	struct goodix_touch_data touch_data;
	struct goodix_pen_data pen_data;
};

/*
 * struct goodix_ts_version - firmware version
 * @valid: whether these information is valid
 * @pid: product id string
 * @vid: firmware version code
 * @cid: customer id code
 * @sensor_id: sendor id
 */
struct goodix_ts_version {
	bool valid;
	char pid[GOODIX_PID_MAX_LEN];
	char vid[GOODIX_VID_MAX_LEN];
	u8 cid;
	u8 sensor_id;
};

struct goodix_ts_regs {
	u16 version_base;
	u8 version_len;

	u16 pid;
	u8 pid_len;

	u16 vid;
	u8 vid_len;

	u16 sensor_id;
	u8 sensor_id_mask;

	u16 cfg_addr;
	u16 esd;
	u16 command;
	u16 coor;
	u16 fw_request;
	u16 proximity;
};

static const struct goodix_ts_regs goodix_ts_regs_normandy = {
	version_base: 17708,
	version_len: 72,
	pid: 17717,
	pid_len: 4,
	vid: 17725,
	vid_len: 4,
	sensor_id: 17729,
	sensor_id_mask: 15,
	cfg_addr: 28536,
	esd: 12531,
	command: 28520,
	coor: 16640,
	fw_request: 0,
	proximity: 0,
};

static const struct goodix_ts_regs goodix_ts_regs_yellowstone = {
	version_base: 16404,
	version_len: 135,
	pid: 16418,
	pid_len: 4,
	vid: 16426,
	vid_len: 4,
	sensor_id: 16431,
	sensor_id_mask: 15,
	cfg_addr: 38648,
	esd: 16742,
	command: 16736,
	coor: 16768,
	fw_request: 16768,
	proximity: 16770,
};

/*
 * struct goodix_ts_device - ts device data
 * @name: device name
 * @version: reserved
 * @bus_type: i2c or spi
 * @ic_type: normandy or yellowstone
 * @cfg_bin_state: see enum goodix_cfg_bin_state
 * @fw_uptodate: set to 1 after do fw update
 * @board_data: board data obtained from dts
 * @hw_ops: hardware operations
 * @chip_version: firmware version information
 * @sleep_cmd: sleep commang
 * @dev: device pointer,may be a i2c or spi device
 * @of_node: device node
 */
struct goodix_ts_device {
	char *name;
	int version;
	int bus_type;
	int ic_type;
	struct goodix_ts_regs reg;
	struct goodix_ts_board_data board_data;
	const struct goodix_ts_hw_ops *hw_ops;

	struct goodix_ts_version chip_version;
	struct device *dev;
};

/*
 * struct goodix_ts_hw_ops -  hardware operations
 * @init: hardware initialization
 * @reset: hardware reset
 * @read: read data from touch device
 * @write: write data to touch device
 * @send_cmd: send command to touch device
 * @read_version: read firmware version
 * @event_handler: touch event handler
 * @suspend: put touch device into low power mode
 * @resume: put touch device into working mode
 */
struct goodix_ts_hw_ops {
	int (*init)(struct goodix_ts_device *dev);
	int (*dev_confirm)(struct goodix_ts_device *ts_dev);
	int (*reset)(struct goodix_ts_device *dev);
	int (*read)(struct goodix_ts_device *dev, unsigned int addr,
		    unsigned char *data, unsigned int len);
	int (*write)(struct goodix_ts_device *dev, unsigned int addr,
		     unsigned char *data, unsigned int len);
	int (*read_trans)(struct goodix_ts_device *dev, unsigned int addr,
			  unsigned char *data, unsigned int len);
	int (*write_trans)(struct goodix_ts_device *dev, unsigned int addr,
			   unsigned char *data, unsigned int len);
	int (*send_cmd)(struct goodix_ts_device *dev,
			struct goodix_ts_cmd *cmd);
	int (*read_version)(struct goodix_ts_device *dev,
			    struct goodix_ts_version *version);
	int (*event_handler)(struct goodix_ts_device *dev,
			     struct goodix_ts_event *ts_event);
	int (*check_hw)(struct goodix_ts_device *dev);
	int (*suspend)(struct goodix_ts_device *dev);
	int (*resume)(struct goodix_ts_device *dev);
};

/*
 * struct goodix_ts_esd - esd protector structure
 * @esd_work: esd delayed work
 * @esd_on: 1 - turn on esd protection, 0 - turn
 *  off esd protection
 */
struct goodix_ts_esd {
	struct delayed_work esd_work;
	struct notifier_block esd_notifier;
	struct goodix_ts_core *ts_core;
	atomic_t esd_on;
};

/*
 * struct godix_ts_core - core layer data struct
 * @initialized: indicate core state, 1 ok, 0 bad
 * @pdev: core layer platform device
 * @ts_dev: hardware layer touch device
 * @input_dev: input device
 * @avdd: analog regulator
 * @pinctrl: pinctrl handler
 * @pin_sta_active: active/normal pin state
 * @pin_sta_suspend: suspend/sleep pin state
 * @ts_event: touch event data struct
 * @power_on: power on/off flag
 * @irq: irq number
 * @irq_enabled: irq enabled/disabled flag
 * @suspended: suspend/resume flag
 * @ts_esd: esd protector structure
 * @fb_notifier: framebuffer notifier
 * @early_suspend: early suspend
 */
struct goodix_ts_core {
	int initialized;
	struct platform_device *pdev;
	struct goodix_ts_device *ts_dev;
	struct input_dev *input_dev;
	struct input_dev *pen_dev;

	struct regulator *avdd;
#ifdef CONFIG_PINCTRL
	struct pinctrl *pinctrl;
	struct pinctrl_state *pin_sta_active;
	struct pinctrl_state *pin_sta_suspend;
#endif
	struct goodix_ts_event ts_event;
	unsigned int avdd_load;
	int power_on;
	int irq;
	size_t irq_trig_cnt;

	atomic_t irq_enabled;
	atomic_t suspended;

	struct goodix_ts_esd ts_esd;

#ifdef CONFIG_FB
	struct notifier_block fb_notifier;
#elif defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend early_suspend;
#endif
};

/*
 * get board data pointer
 */
static inline struct goodix_ts_board_data *
board_data(struct goodix_ts_core *core)
{
	if (!core || !core->ts_dev)
		return NULL;
	return &(core->ts_dev->board_data);
}

/*
 * get touch device pointer
 */
static inline struct goodix_ts_device *ts_device(struct goodix_ts_core *core)
{
	if (!core)
		return NULL;
	return core->ts_dev;
}

/*
 * get touch hardware operations pointer
 */
static inline const struct goodix_ts_hw_ops *
ts_hw_ops(struct goodix_ts_core *core)
{
	if (!core || !core->ts_dev)
		return NULL;
	return core->ts_dev->hw_ops;
}

/*
 * checksum helper functions
 * checksum can be u8/le16/be16/le32/be32 format
 * NOTE: the caller should be responsible for the
 * legality of @data and @size parameters, so be
 * careful when call these functions.
 */
static inline u8 checksum_u8(u8 *data, u32 size)
{
	u8 checksum = 0;
	u32 i;

	for (i = 0; i < size; i++)
		checksum += data[i];
	return checksum;
}

/* cal u8 data checksum for yellowston */
static inline u16 checksum_u8_ys(u8 *data, u32 size)
{
	u16 checksum = 0;
	u32 i;

	for (i = 0; i < size - 2; i++)
		checksum += data[i];
	return checksum - (data[size - 2] << 8 | data[size - 1]);
}

static inline u16 checksum_le16(u8 *data, u32 size)
{
	u16 checksum = 0;
	u32 i;

	for (i = 0; i < size; i += 2)
		checksum += le16_to_cpup((__le16 *)(data + i));
	return checksum;
}

static inline u16 checksum_be16(u8 *data, u32 size)
{
	u16 checksum = 0;
	u32 i;

	for (i = 0; i < size; i += 2)
		checksum += be16_to_cpup((__be16 *)(data + i));
	return checksum;
}

static inline u32 checksum_le32(u8 *data, u32 size)
{
	u32 checksum = 0;
	u32 i;

	for (i = 0; i < size; i += 4)
		checksum += le32_to_cpup((__le32 *)(data + i));
	return checksum;
}

static inline u32 checksum_be32(u8 *data, u32 size)
{
	u32 checksum = 0;
	u32 i;

	for (i = 0; i < size; i += 4)
		checksum += be32_to_cpup((__be32 *)(data + i));
	return checksum;
}

/*
 * errno define
 * Note:
 *	1. bus read/write functions defined in hardware
 *	  layer code(e.g. goodix_xxx_i2c.c) *must* return
 *	  -EBUS if failed to transfer data on bus.
 */
#define EBUS 1000
#define ETIMEOUT 1001
#define ECHKSUM 1002
#define EMEMCMP 1003

//#define CONFIG_GOODIX_DEBUG
/* log macro */
#define ts_info(fmt, arg...) \
	pr_info("[GTP-INF][%s:%d] " fmt "\n", __func__, __LINE__, ##arg)
#define ts_err(fmt, arg...) \
	pr_err("[GTP-ERR][%s:%d] " fmt "\n", __func__, __LINE__, ##arg)
#define boot_log(fmt, arg...) g_info(fmt, ##arg)
#ifdef CONFIG_GOODIX_DEBUG
#define ts_debug(fmt, arg...) \
	pr_info("[GTP-DBG][%s:%d] " fmt "\n", __func__, __LINE__, ##arg)
#else
#define ts_debug(fmt, arg...) \
	do {                  \
	} while (0)
#endif

/**
 * goodix_ts_irq_enable - Enable/Disable a irq

 * @core_data: pointer to touch core data
 * enable: enable or disable irq
 * return: 0 ok, <0 failed
 */
int goodix_ts_irq_enable(struct goodix_ts_core *core_data, bool enable);

/**
 *  * goodix_ts_power_on - Turn on power to the touch device
 *   * @core_data: pointer to touch core data
 *    * return: 0 ok, <0 failed
 *     */
int goodix_ts_power_on(struct goodix_ts_core *core_data);

/**
 * goodix_ts_power_off - Turn off power to the touch device
 * @core_data: pointer to touch core data
 * return: 0 ok, <0 failed
 */
int goodix_ts_power_off(struct goodix_ts_core *core_data);

int goodix_ts_irq_setup(struct goodix_ts_core *core_data);

int goodix_ts_esd_init(struct goodix_ts_core *core);

int goodix_ts_fb_notifier_callback(struct notifier_block *self,
				   unsigned long event, void *data);

int goodix_ts_core_init(void);

#endif
