/*
 * Marvell 88E6171 switch driver
 *
 * Copyright (c) 2014 Claudio Leite <leitec@staticky.com>
 *
 * Based on code (c) 2008 Felix Fietkau <nbd@openwrt.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License v2 as published by the
 * Free Software Foundation
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/mii.h>
#include <linux/phy.h>
#include <linux/delay.h>
#include <linux/switch.h>
#include <linux/device.h>

#include "mvsw6171.h"

MODULE_DESCRIPTION("Marvell 88E6171 Switch driver");
MODULE_AUTHOR("Claudio Leite <leitec@staticky.com>");
MODULE_LICENSE("GPL v2");

/*
 * Register access is done through direct or indirect addressing,
 * depending on how the switch is physically connected.
 *
 * Direct addressing: all port and global registers directly
 *   accessible via an address/register pair
 *
 * Indirect addressing: switch is mapped at a single address,
 *   port and global registers accessible via a single command/data
 *   register pair
 */

static int
mvsw6171_wait_mask_raw(struct phy_device *pdev, int addr,
		int reg, u16 mask, u16 val)
{
	int i = 100;
	u16 r;

	do {
		r = pdev->bus->read(pdev->bus, addr, reg);
		if ((r & mask) == val)
			return 0;
	} while (--i > 0);

	return -ETIMEDOUT;
}

static u16
r16(struct phy_device *pdev, bool direct, int addr, int reg)
{
	u16 ind_addr, ret;

	if (direct)
		return pdev->bus->read(pdev->bus, addr, reg);

	mvsw6171_wait_mask_raw(pdev, pdev->addr, 0, 0x8000, 0);

	ind_addr = 0x9800 | (addr << 5) | reg;
	pdev->bus->write(pdev->bus, pdev->addr, 0, ind_addr);

	mvsw6171_wait_mask_raw(pdev, pdev->addr, 0, 0x8000, 0);

	ret = pdev->bus->read(pdev->bus, pdev->addr, 1);

	pr_info("read_indirect: ind_addr=%04x val=%04x\n", ind_addr, ret);

	return ret;
}

static void
w16(struct phy_device *pdev, bool direct, int addr,
		int reg, u16 val)
{
	u16 ind_addr;

	if (direct) {
		pdev->bus->write(pdev->bus, addr, reg, val);
		return;
	}

	mvsw6171_wait_mask_raw(pdev, pdev->addr, 0, 0x8000, 0);

	pdev->bus->write(pdev->bus, pdev->addr, 1, val);

	mvsw6171_wait_mask_raw(pdev, pdev->addr, 0, 0x8000, 0);

	ind_addr = 0x9400 | (addr << 5) | reg;
	pdev->bus->write(pdev->bus, pdev->addr, 0, ind_addr);
	pr_info("write_indirect: ind_addr=%04x val=%04x\n", ind_addr, val);
}

/* swconfig support */

static inline u16
sr16(struct switch_dev *dev, int addr, int reg)
{
	struct mvsw6171_state *state = get_state(dev);

	return r16(state->pdev, state->direct, addr, reg);
}

static inline void
sw16(struct switch_dev *dev, int addr, int reg, u16 val)
{
	struct mvsw6171_state *state = get_state(dev);

	w16(state->pdev, state->direct, addr, reg, val);
}

static int
mvsw6171_wait_mask_s(struct switch_dev *dev, int addr,
		int reg, u16 mask, u16 val)
{
	int i = 100;
	u16 r;

	do {
		r = sr16(dev, addr, reg) & mask;
		if (r == val)
			return 0;
	} while (--i > 0);

	return -ETIMEDOUT;
}

static int
mvsw6171_get_port_mask(struct switch_dev *dev,
		const struct switch_attr *attr, struct switch_val *val)
{
	struct mvsw6171_state *state = get_state(dev);
	char *buf = state->buf;
	int port, len, i;
	u16 reg;

	port = val->port_vlan;
	reg = sr16(dev, MV_PORTREG(VLANMAP, port)) & MV_PORTS_MASK;

	len = sprintf(buf, "0x%04x: ", reg);

	for (i = 0; i < MV_PORTS; i++) {
		if (reg & (1 << i))
			len += sprintf(buf + len, "%d ", i);
		else if (i == port)
			len += sprintf(buf + len, "(%d) ", i);
	}

	val->value.s = buf;

	return 0;
}

static int
mvsw6171_get_port_qmode(struct switch_dev *dev,
		const struct switch_attr *attr, struct switch_val *val)
{
	struct mvsw6171_state *state = get_state(dev);

	val->value.i = state->ports[val->port_vlan].qmode;

	return 0;
}

static int
mvsw6171_set_port_qmode(struct switch_dev *dev,
		const struct switch_attr *attr, struct switch_val *val)
{
	struct mvsw6171_state *state = get_state(dev);

	state->ports[val->port_vlan].qmode = val->value.i;

	return 0;
}

static int
mvsw6171_get_pvid(struct switch_dev *dev, int port, int *val)
{
	struct mvsw6171_state *state = get_state(dev);

	*val = state->ports[port].pvid;

	return 0;
}

static int
mvsw6171_set_pvid(struct switch_dev *dev, int port, int val)
{
	struct mvsw6171_state *state = get_state(dev);

	if (val < 0 || val >= MV_VLANS)
		return -EINVAL;

	state->ports[port].pvid = (u16)val;

	return 0;
}

static int
mvsw6171_get_port_status(struct switch_dev *dev,
		const struct switch_attr *attr, struct switch_val *val)
{
	struct mvsw6171_state *state = get_state(dev);
	char *buf = state->buf;
	u16 status, speed;
	int len;

	status = sr16(dev, MV_PORTREG(STATUS, val->port_vlan));
	speed = (status & MV_PORT_STATUS_SPEED_MASK) >>
			MV_PORT_STATUS_SPEED_SHIFT;

	len = sprintf(buf, "link: ");
	if (status & MV_PORT_STATUS_LINK) {
		len += sprintf(buf + len, "up, speed: ");

		switch (speed) {
		case MV_PORT_STATUS_SPEED_10:
			len += sprintf(buf + len, "10");
			break;
		case MV_PORT_STATUS_SPEED_100:
			len += sprintf(buf + len, "100");
			break;
		case MV_PORT_STATUS_SPEED_1000:
			len += sprintf(buf + len, "1000");
			break;
		}

		len += sprintf(buf + len, " Mbps, duplex: ");

		if (status & MV_PORT_STATUS_FDX)
			len += sprintf(buf + len, "full");
		else
			len += sprintf(buf + len, "half");
	} else {
		len += sprintf(buf + len, "down");
	}

	val->value.s = buf;

	return 0;
}

static int
mvsw6171_get_port_speed(struct switch_dev *dev,
		const struct switch_attr *attr, struct switch_val *val)
{
	u16 status, speed;

	status = sr16(dev, MV_PORTREG(STATUS, val->port_vlan));
	speed = (status & MV_PORT_STATUS_SPEED_MASK) >>
			MV_PORT_STATUS_SPEED_SHIFT;

	val->value.i = 0;

	if (status & MV_PORT_STATUS_LINK) {
		switch (speed) {
		case MV_PORT_STATUS_SPEED_10:
			val->value.i = 10;
			break;
		case MV_PORT_STATUS_SPEED_100:
			val->value.i = 100;
			break;
		case MV_PORT_STATUS_SPEED_1000:
			val->value.i = 1000;
			break;
		}
	}

	return 0;
}

static int mvsw6171_get_vlan_ports(struct switch_dev *dev,
		struct switch_val *val)
{
	struct mvsw6171_state *state = get_state(dev);
	int i, j, mode, vno;

	vno = val->port_vlan;

	if (vno <= 0 || vno >= dev->vlans)
		return -EINVAL;

	for (i = 0, j = 0; i < dev->ports; i++) {
		if (state->vlans[vno].mask & (1 << i)) {
			val->value.ports[j].id = i;

			mode = (state->vlans[vno].port_mode >> (i * 4)) & 0xf;
			if (mode == MV_VTUCTL_EGRESS_TAGGED)
				val->value.ports[j].flags =
					(1 << SWITCH_PORT_FLAG_TAGGED);
			else
				val->value.ports[j].flags = 0;

			j++;
		}
	}

	val->len = j;

	return 0;
}

static int mvsw6171_set_vlan_ports(struct switch_dev *dev,
		struct switch_val *val)
{
	struct mvsw6171_state *state = get_state(dev);
	int i, mode, pno, vno;

	vno = val->port_vlan;

	if (vno <= 0 || vno >= dev->vlans)
		return -EINVAL;

	state->vlans[vno].mask = 0;
	state->vlans[vno].port_mode = 0;

	if(state->vlans[vno].vid == 0)
		state->vlans[vno].vid = vno;

	for (i = 0; i < val->len; i++) {
		pno = val->value.ports[i].id;

		state->vlans[vno].mask |= (1 << pno);
		if (val->value.ports[i].flags &
				(1 << SWITCH_PORT_FLAG_TAGGED))
			mode = MV_VTUCTL_EGRESS_TAGGED;
		else
			mode = MV_VTUCTL_EGRESS_UNTAGGED;

		state->vlans[vno].port_mode |= mode << (pno * 4);
	}

	/*
	 * DISCARD is nonzero, so it must be explicitly
	 * set on ports not in the VLAN.
	 */
	for (i = 0; i < dev->ports; i++)
		if (!(state->vlans[vno].mask & (1 << i)))
			state->vlans[vno].port_mode |=
				MV_VTUCTL_DISCARD << (i * 4);

	return 0;
}

static int mvsw6171_get_vlan_port_based(struct switch_dev *dev,
		const struct switch_attr *attr, struct switch_val *val)
{
	struct mvsw6171_state *state = get_state(dev);
	int vno = val->port_vlan;

	if (vno <= 0 || vno >= dev->vlans)
		return -EINVAL;

	if (state->vlans[vno].port_based)
		val->value.i = 1;
	else
		val->value.i = 0;

	return 0;
}

static int mvsw6171_set_vlan_port_based(struct switch_dev *dev,
		const struct switch_attr *attr, struct switch_val *val)
{
	struct mvsw6171_state *state = get_state(dev);
	int vno = val->port_vlan;

	if (vno <= 0 || vno >= dev->vlans)
		return -EINVAL;

	if (val->value.i == 1)
		state->vlans[vno].port_based = true;
	else
		state->vlans[vno].port_based = false;

	return 0;
}

static int mvsw6171_get_vid(struct switch_dev *dev,
		const struct switch_attr *attr, struct switch_val *val)
{
	struct mvsw6171_state *state = get_state(dev);
	int vno = val->port_vlan;

	if (vno <= 0 || vno >= dev->vlans)
		return -EINVAL;

	val->value.i = state->vlans[vno].vid;

	return 0;
}

static int mvsw6171_set_vid(struct switch_dev *dev,
		const struct switch_attr *attr, struct switch_val *val)
{
	struct mvsw6171_state *state = get_state(dev);
	int vno = val->port_vlan;

	if (vno <= 0 || vno >= dev->vlans)
		return -EINVAL;

	state->vlans[vno].vid = val->value.i;

	return 0;
}

static int mvsw6171_get_enable_vlan(struct switch_dev *dev,
		const struct switch_attr *attr, struct switch_val *val)
{
	struct mvsw6171_state *state = get_state(dev);

	val->value.i = state->vlan_enabled;

	return 0;
}

static int mvsw6171_set_enable_vlan(struct switch_dev *dev,
		const struct switch_attr *attr, struct switch_val *val)
{
	struct mvsw6171_state *state = get_state(dev);

	state->vlan_enabled = val->value.i;

	return 0;
}

static int mvsw6171_vtu_program(struct switch_dev *dev)
{
	struct mvsw6171_state *state = get_state(dev);
	u16 v1, v2;
	int i;

	/* Flush */
	mvsw6171_wait_mask_s(dev, MV_GLOBALREG(VTU_OP),
			MV_VTUOP_INPROGRESS, 0);
	sw16(dev, MV_GLOBALREG(VTU_OP),
			MV_VTUOP_INPROGRESS | MV_VTUOP_VALID);

	/* Write VLAN table */
	for (i = 1; i < dev->vlans; i++) {
		if (state->vlans[i].mask == 0 ||
				state->vlans[i].vid == 0 ||
				state->vlans[i].port_based == true)
			continue;

		mvsw6171_wait_mask_s(dev, MV_GLOBALREG(VTU_OP),
				MV_VTUOP_INPROGRESS, 0);

		sw16(dev, MV_GLOBALREG(VTU_VID),
				MV_VTUOP_VALID | state->vlans[i].vid);

		v1 = (u16)(state->vlans[i].port_mode & 0xffff);
		v2 = (u16)((state->vlans[i].port_mode >> 16) & 0xffff);

		sw16(dev, MV_GLOBALREG(VTU_DATA1), v1);
		sw16(dev, MV_GLOBALREG(VTU_DATA2), v2);

		sw16(dev, MV_GLOBALREG(VTU_OP),
				MV_VTUOP_INPROGRESS | MV_VTUOP_LOAD);
		mvsw6171_wait_mask_s(dev, MV_GLOBALREG(VTU_OP),
				MV_VTUOP_INPROGRESS, 0);
	}

	return 0;
}

static void mvsw6171_vlan_port_config(struct switch_dev *dev, int vno)
{
	struct mvsw6171_state *state = get_state(dev);
	int i, mode;

	for (i = 0; i < dev->ports; i++) {
		if (!(state->vlans[vno].mask & (1 << i)))
			continue;

		mode = (state->vlans[vno].port_mode >> (i * 4)) & 0xf;

		if(mode != MV_VTUCTL_EGRESS_TAGGED)
			state->ports[i].pvid = state->vlans[vno].vid;

		if (state->vlans[vno].port_based)
			state->ports[i].mask |= state->vlans[vno].mask;
		else
			state->ports[i].qmode = MV_8021Q_MODE_SECURE;
	}
}

static int mvsw6171_update_state(struct switch_dev *dev)
{
	struct mvsw6171_state *state = get_state(dev);
	int i;
	u16 reg;

	if (!state->registered)
		return -EINVAL;

	mvsw6171_vtu_program(dev);

	/*
	 * Set 802.1q-only mode if vlan_enabled is true.
	 *
	 * Without this, even if 802.1q is enabled for
	 * a port/VLAN, it still depends on the port-based
	 * VLAN mask being set.
	 *
	 * With this setting, port-based VLANs are still
	 * functional, provided the VID is not in the VTU.
	 */
	reg = sr16(dev, MV_GLOBAL2REG(SDET_POLARITY));

	if (state->vlan_enabled)
		reg |= MV_8021Q_VLAN_ONLY;
	else
		reg &= ~MV_8021Q_VLAN_ONLY;

	sw16(dev, MV_GLOBAL2REG(SDET_POLARITY), reg);

	/*
	 * Set port-based VLAN masks on each port
	 * based only on VLAN definitions known to
	 * the driver (i.e. in state).
	 *
	 * This means any pre-existing port mapping is
	 * wiped out once our driver is initialized.
	 */
	for (i = 0; i < dev->ports; i++) {
		state->ports[i].mask = 0;
		state->ports[i].qmode = MV_8021Q_MODE_DISABLE;
	}

	for (i = 0; i < dev->vlans; i++)
		mvsw6171_vlan_port_config(dev, i);

	for (i = 0; i < dev->ports; i++) {
		reg = sr16(dev, MV_PORTREG(VLANID, i)) & ~MV_PVID_MASK;
		reg |= state->ports[i].pvid;
		sw16(dev, MV_PORTREG(VLANID, i), reg);

		state->ports[i].mask &= ~(1 << i);

		reg = sr16(dev, MV_PORTREG(VLANMAP, i)) & ~MV_PORTS_MASK;
		reg |= state->ports[i].mask;
		sw16(dev, MV_PORTREG(VLANMAP, i), reg);

		reg = sr16(dev, MV_PORTREG(CONTROL2, i)) &
			~MV_8021Q_MODE_MASK;
		reg |= state->ports[i].qmode << MV_8021Q_MODE_SHIFT;
		sw16(dev, MV_PORTREG(CONTROL2, i), reg);
	}

	return 0;
}

static int mvsw6171_apply(struct switch_dev *dev)
{
	return mvsw6171_update_state(dev);
}

static int mvsw6171_reset(struct switch_dev *dev)
{
	struct mvsw6171_state *state = get_state(dev);
	int i;
	u16 reg;

	/* Disable all ports before reset */
	for (i = 0; i < dev->ports; i++) {
		reg = sr16(dev, MV_PORTREG(CONTROL, i)) &
			~MV_PORTCTRL_ENABLED;
		sw16(dev, MV_PORTREG(CONTROL, i), reg);
	}

	reg = sr16(dev, MV_GLOBALREG(CONTROL)) | MV_CONTROL_RESET;

	sw16(dev, MV_GLOBALREG(CONTROL), reg);
	if (mvsw6171_wait_mask_s(dev, MV_GLOBALREG(CONTROL),
				MV_CONTROL_RESET, 0) < 0)
		return -ETIMEDOUT;

	for (i = 0; i < dev->ports; i++) {
		state->ports[i].qmode = 0;
		state->ports[i].mask = 0;
		state->ports[i].pvid = 0;

		/* Force flow control off */
		reg = sr16(dev, MV_PORTREG(FORCE, i)) & ~MV_FORCE_FC_MASK;
		reg |= MV_FORCE_FC_DISABLE;
		sw16(dev, MV_PORTREG(FORCE, i), reg);

		/* Set port association vector */
		sw16(dev, MV_PORTREG(ASSOC, i), (1 << i));
	}

	for (i = 0; i < dev->vlans; i++) {
		state->vlans[i].port_based = false;
		state->vlans[i].mask = 0;
		state->vlans[i].vid = 0;
		state->vlans[i].port_mode = 0;
	}

	state->vlan_enabled = 0;

	mvsw6171_update_state(dev);

	/* Re-enable ports */
	for (i = 0; i < dev->ports; i++) {
		reg = sr16(dev, MV_PORTREG(CONTROL, i)) |
			MV_PORTCTRL_ENABLED;
		sw16(dev, MV_PORTREG(CONTROL, i), reg);
	}

	return 0;
}

enum {
	MVSW6171_ENABLE_VLAN,
};

enum {
	MVSW6171_VLAN_PORT_BASED,
	MVSW6171_VLAN_ID,
};

enum {
	MVSW6171_PORT_MASK,
	MVSW6171_PORT_QMODE,
	MVSW6171_PORT_STATUS,
	MVSW6171_PORT_LINK,
};

static const struct switch_attr mvsw6171_global[] = {
	[MVSW6171_ENABLE_VLAN] = {
		.id = MVSW6171_ENABLE_VLAN,
		.type = SWITCH_TYPE_INT,
		.name = "enable_vlan",
		.description = "Enable 802.1q VLAN support",
		.get = mvsw6171_get_enable_vlan,
		.set = mvsw6171_set_enable_vlan,
	},
};

static const struct switch_attr mvsw6171_vlan[] = {
	[MVSW6171_VLAN_PORT_BASED] = {
		.id = MVSW6171_VLAN_PORT_BASED,
		.type = SWITCH_TYPE_INT,
		.name = "port_based",
		.description = "Use port-based (non-802.1q) VLAN only",
		.get = mvsw6171_get_vlan_port_based,
		.set = mvsw6171_set_vlan_port_based,
	},
	[MVSW6171_VLAN_ID] = {
		.id = MVSW6171_VLAN_ID,
		.type = SWITCH_TYPE_INT,
		.name = "vid",
		.description = "Get/set VLAN ID",
		.get = mvsw6171_get_vid,
		.set = mvsw6171_set_vid,
	},
};

static const struct switch_attr mvsw6171_port[] = {
	[MVSW6171_PORT_MASK] = {
		.id = MVSW6171_PORT_MASK,
		.type = SWITCH_TYPE_STRING,
		.description = "Port-based VLAN mask",
		.name = "mask",
		.get = mvsw6171_get_port_mask,
		.set = NULL,
	},
	[MVSW6171_PORT_QMODE] = {
		.id = MVSW6171_PORT_QMODE,
		.type = SWITCH_TYPE_INT,
		.description = "802.1q mode: 0=off/1=fallback/2=check/3=secure",
		.name = "qmode",
		.get = mvsw6171_get_port_qmode,
		.set = mvsw6171_set_port_qmode,
	},
	[MVSW6171_PORT_STATUS] = {
		.id = MVSW6171_PORT_STATUS,
		.type = SWITCH_TYPE_STRING,
		.description = "Return port status",
		.name = "status",
		.get = mvsw6171_get_port_status,
		.set = NULL,
	},
	[MVSW6171_PORT_LINK] = {
		.id = MVSW6171_PORT_LINK,
		.type = SWITCH_TYPE_INT,
		.description = "Get link speed",
		.name = "link",
		.get = mvsw6171_get_port_speed,
		.set = NULL,
	},
};

static const struct switch_dev_ops mvsw6171_ops = {
	.attr_global = {
		.attr = mvsw6171_global,
		.n_attr = ARRAY_SIZE(mvsw6171_global),
	},
	.attr_vlan = {
		.attr = mvsw6171_vlan,
		.n_attr = ARRAY_SIZE(mvsw6171_vlan),
	},
	.attr_port = {
		.attr = mvsw6171_port,
		.n_attr = ARRAY_SIZE(mvsw6171_port),
	},
	.get_port_pvid = mvsw6171_get_pvid,
	.set_port_pvid = mvsw6171_set_pvid,
	.get_vlan_ports = mvsw6171_get_vlan_ports,
	.set_vlan_ports = mvsw6171_set_vlan_ports,
	.apply_config = mvsw6171_apply,
	.reset_switch = mvsw6171_reset,
};

/* end swconfig stuff */

static int
mvsw6171_read_status(struct phy_device *pdev)
{
	pdev->autoneg = AUTONEG_DISABLE;

	pdev->speed = SPEED_1000;
	pdev->duplex = DUPLEX_FULL;

	pdev->pause = 0;
	pdev->asym_pause = 0;

	pdev->link = 1;

	return 0;
}

static int
mvsw6171_config_aneg(struct phy_device *pdev)
{
	return 0;
}

static int
mvsw6171_aneg_done(struct phy_device *pdev)
{
	return BMSR_ANEGCOMPLETE;
}

static int mvsw6171_config_init(struct phy_device *pdev)
{
	struct mvsw6171_state *state = pdev->priv;
	struct net_device *ndev = pdev->attached_dev;
	int err;

	err = register_switch(&state->dev, ndev);
	if (err < 0)
		return err;

	state->registered = true;
	state->cpu_port0 = state->dev.cpu_port;
	state->cpu_port1 = -1;
	state->pdev = pdev;

	pdev->irq = PHY_POLL;
	pdev->supported = SUPPORTED_1000baseT_Full;
	pdev->advertising = SUPPORTED_1000baseT_Full;

	return 0;
}

static int
mvsw6171_update_link(struct phy_device *pdev)
{
	pdev->link = 1;

	return 0;
}

static void
mvsw6171_remove(struct phy_device *pdev)
{
	struct mvsw6171_state *state = pdev->priv;

	if (state->registered)
		unregister_switch(&state->dev);

	kfree(state);
}

static int
mvsw6171_probe(struct phy_device *pdev)
{
	struct mvsw6171_state *state;
	struct switch_dev *dev;

	if (pdev->addr != MV_BASE)
		return -ENODEV;

	state = kzalloc(sizeof(*state), GFP_KERNEL);
	if (!state)
		return -ENOMEM;

	dev = &state->dev;
	pdev->priv = state;

	dev->vlans = MV_VLANS;
	dev->cpu_port = MV_CPUPORT;
	dev->ports = MV_PORTS;
	dev->name = "MV88E6171";
	dev->ops = &mvsw6171_ops;

	pr_info("mvsw6171: Found %s at %s\n", dev->name, dev_name(&pdev->dev));

	if (pdev->phy_id == MVSW6171_MAGIC_INDIRECT)
		state->direct = false;
	else
		state->direct = true;

	pr_info("mvsw6171: Using %sdirect addressing\n",
			(state->direct ? "" : "in"));

	return 0;
}

static int
mvsw6171_fixup(struct phy_device *pdev)
{
	int i;
	u16 reg;

	pr_info("phy_fixup: %d\n", pdev->addr);

	if (pdev->addr != MV_BASE)
		return 0;

	pr_info("mvsw6171: MDIO register dump:\n");
	for (i = 0; i < 32; i++) {
		reg = r16(pdev, true, pdev->addr, i);
		pr_info("mvsw6171:  %02x  %04x\n", i, reg);
	}

	/* Try using direct mode first */
	reg = r16(pdev, true, MV_PORTREG(IDENT, 0)) & MV_IDENT_MASK;
	if (reg == MV_IDENT_VALUE) {
		pdev->phy_id = MVSW6171_MAGIC;
		return 0;
	}

	/* Try again with indirect mode */
	reg = r16(pdev, false, MV_PORTREG(IDENT, 0)) & MV_IDENT_MASK;
	if (reg == MV_IDENT_VALUE)
		pdev->phy_id = MVSW6171_MAGIC_INDIRECT;

	return 0;
}

static struct phy_driver mvsw6171_driver = {
	.name		= "Marvell 88E6171",
	.phy_id		= MVSW6171_MAGIC,
	.phy_id_mask	= 0x0fffffff,
	.features	= PHY_BASIC_FEATURES,
	.probe		= &mvsw6171_probe,
	.remove		= &mvsw6171_remove,
	.config_init	= &mvsw6171_config_init,
	.config_aneg	= &mvsw6171_config_aneg,
	.aneg_done      = &mvsw6171_aneg_done,
	.update_link    = &mvsw6171_update_link,
	.read_status	= &mvsw6171_read_status,
	.driver		= { .owner = THIS_MODULE,},
};

static int __init
mvsw6171_init(void)
{
	phy_register_fixup_for_id(PHY_ANY_ID, mvsw6171_fixup);
	return phy_driver_register(&mvsw6171_driver);
}

static void __exit
mvsw6171_exit(void)
{
	phy_driver_unregister(&mvsw6171_driver);
}

module_init(mvsw6171_init);
module_exit(mvsw6171_exit);
