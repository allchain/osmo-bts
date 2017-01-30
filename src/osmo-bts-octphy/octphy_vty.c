/* VTY interface for osmo-bts OCTPHY integration */

/* (C) 2015-2016 by Harald Welte <laforge@gnumonks.org>
 *
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <ctype.h>

#include <arpa/inet.h>

#include <osmocom/core/msgb.h>
#include <osmocom/core/talloc.h>
#include <osmocom/core/select.h>
#include <osmocom/core/rate_ctr.h>
#include <osmocom/core/macaddr.h>

#include <osmocom/vty/vty.h>
#include <osmocom/vty/command.h>
#include <osmocom/vty/misc.h>

#include <osmo-bts/gsm_data.h>
#include <osmo-bts/phy_link.h>
#include <osmo-bts/logging.h>
#include <osmo-bts/vty.h>

#include "l1_if.h"
#include "l1_utils.h"
#include "octphy_hw_api.h"

#define TRX_STR "Transceiver related commands\n" "TRX number\n"

#define SHOW_TRX_STR				\
	SHOW_STR				\
	TRX_STR

#define OCT_STR	"OCTPHY Um interface\n"

static struct gsm_bts *vty_bts;

/* configuration */

DEFUN(cfg_phy_hwaddr, cfg_phy_hwaddr_cmd,
	"octphy hw-addr HWADDR",
	OCT_STR "Configure the hardware addess of the OCTPHY\n"
	"hardware address in aa:bb:cc:dd:ee:ff format\n")
{
	struct phy_link *plink = vty->index;
	int rc;

	if (plink->state != PHY_LINK_SHUTDOWN) {
		vty_out(vty, "Can only reconfigure a PHY link that is down%s",
			VTY_NEWLINE);
		return CMD_WARNING;
	}

	rc = osmo_macaddr_parse(plink->u.octphy.phy_addr.sll_addr, argv[0]);
	if (rc < 0)
		return CMD_WARNING;

	return CMD_SUCCESS;
}

DEFUN(cfg_phy_netdev, cfg_phy_netdev_cmd,
	"octphy net-device NAME",
	OCT_STR "Configure the hardware device towards the OCTPHY\n"
	"Ethernet device name\n")
{
	struct phy_link *plink = vty->index;

	if (plink->state != PHY_LINK_SHUTDOWN) {
		vty_out(vty, "Can only reconfigure a PHY link that is down%s",
			VTY_NEWLINE);
		return CMD_WARNING;
	}

	if (plink->u.octphy.netdev_name)
		talloc_free(plink->u.octphy.netdev_name);
	plink->u.octphy.netdev_name = talloc_strdup(plink, argv[0]);

	return CMD_SUCCESS;
}

DEFUN(cfg_phy_rf_port_idx, cfg_phy_rf_port_idx_cmd,
	"octphy rf-port-index <0-255>",
	OCT_STR "Configure the RF Port for this TRX\n"
	"RF Port Index\n")
{
	struct phy_link *plink = vty->index;

	if (plink->state != PHY_LINK_SHUTDOWN) {
		vty_out(vty, "Can only reconfigure a PHY link that is down%s",
			VTY_NEWLINE);
		return CMD_WARNING;
	}

	plink->u.octphy.rf_port_index = atoi(argv[0]);

	return CMD_SUCCESS;
}

DEFUN(cfg_phy_rx_gain_db, cfg_phy_rx_gain_db_cmd,
	"octphy rx-gain <0-73>",
	OCT_STR "Configure the Rx Gain in dB\n"
	"Rx gain in dB\n")
{
	struct phy_link *plink = vty->index;

	if (plink->state != PHY_LINK_SHUTDOWN) {
		vty_out(vty, "Can only reconfigure a PHY link that is down%s",
			VTY_NEWLINE);
		return CMD_WARNING;
	}

	plink->u.octphy.rx_gain_db = atoi(argv[0]);

	return CMD_SUCCESS;
}

DEFUN(cfg_phy_tx_atten_db, cfg_phy_tx_atten_db_cmd,
	"octphy tx-attenuation (oml|<0-359>)",
	OCT_STR "Set attenuation on transmitted RF\n"
	"Use tx-attenuation according to OML instructions from BSC\n"
	"Fixed tx-attenuation in quarter-dB\n")
{
	struct phy_link *plink = vty->index;

	if (plink->state != PHY_LINK_SHUTDOWN) {
		vty_out(vty, "Can only reconfigure a PHY link that is down%s",
			VTY_NEWLINE);
		return CMD_WARNING;
	}

	if (strcmp(argv[0], "oml") == 0) {
		plink->u.octphy.tx_atten_flag = false;
	} else {
		plink->u.octphy.tx_atten_db = atoi(argv[0]);
		plink->u.octphy.tx_atten_flag = true;
	}

	return CMD_SUCCESS;
}

void show_rf_port_stats_cb(struct msgb *resp, void *data)
{
	struct vty *vty = (struct vty*) data;
	tOCTVC1_HW_MSG_RF_PORT_STATS_RSP *psr;

	if (sizeof(tOCTVC1_HW_MSG_RF_PORT_STATS_RSP) != msgb_l2len(resp)) {
		vty_out(vty,
			"invalid msgb size (%d bytes, expected %zu bytes)%s",
			msgb_l2len(resp),
			sizeof(tOCTVC1_HW_MSG_RF_PORT_STATS_RSP), VTY_NEWLINE);
		return;
	}

	psr = (tOCTVC1_HW_MSG_RF_PORT_STATS_RSP *) msgb_l2(resp);

	vty_out(vty, "%s", VTY_NEWLINE);
	vty_out(vty, "RF-PORT-STATS:%s", VTY_NEWLINE);
	vty_out(vty, "Idx=%d%s", psr->ulPortIndex, VTY_NEWLINE);
	vty_out(vty, "RadioStandard=%s%s",
		get_value_string(radio_std_vals, psr->ulRadioStandard),
		VTY_NEWLINE);
	vty_out(vty, "Rx Bytes=%u%s", psr->RxStats.ulRxByteCnt, VTY_NEWLINE);
	vty_out(vty, "Rx Overflow=%u%s", psr->RxStats.ulRxOverflowCnt,
		VTY_NEWLINE);
	vty_out(vty, "Rx AvgBps=%u%s", psr->RxStats.ulRxAverageBytePerSecond,
		VTY_NEWLINE);
	vty_out(vty, "Rx Period=%u%s", psr->RxStats.ulRxAveragePeriodUs,
		VTY_NEWLINE);
	vty_out(vty, "Rx Freq=%u%s", psr->RxStats.ulFrequencyKhz, VTY_NEWLINE);
	vty_out(vty, "Tx Bytes=%u%s", psr->TxStats.ulTxByteCnt, VTY_NEWLINE);
	vty_out(vty, "Tx Underflow=%u%s", psr->TxStats.ulTxUnderflowCnt,
		VTY_NEWLINE);
	vty_out(vty, "Tx AvgBps=%u%s", psr->TxStats.ulTxAverageBytePerSecond,
		VTY_NEWLINE);
	vty_out(vty, "Tx Period=%u%s", psr->TxStats.ulTxAveragePeriodUs,
		VTY_NEWLINE);
	vty_out(vty, "Tx Freq=%u%s", psr->TxStats.ulFrequencyKhz, VTY_NEWLINE);
}

DEFUN(show_rf_port_stats, show_rf_port_stats_cmd,
	"show phy <0-255> rf-port-stats <0-1>",
	"Show statistics for the RF Port\n"
	"RF Port Number\n")
{
	int phy_nr = atoi(argv[0]);
	struct phy_link *plink = phy_link_by_num(phy_nr);
	static struct octphy_hw_get_cb_data cb_data;

	cb_data.cb = show_rf_port_stats_cb;
	cb_data.data = vty;

	octphy_hw_get_rf_port_stats(plink->u.octphy.hdl, atoi(argv[1]),
				    &cb_data);

	return CMD_SUCCESS;
}

void show_clk_sync_stats_cb(struct msgb *resp, void *data)
{
	struct vty *vty = (struct vty*) data;
	tOCTVC1_HW_MSG_CLOCK_SYNC_MGR_STATS_RSP *csr;

	if (sizeof(tOCTVC1_HW_MSG_CLOCK_SYNC_MGR_STATS_RSP) !=
	    msgb_l2len(resp)) {
		vty_out(vty,
			"invalid msgb size (%d bytes, expected %zu bytes)%s",
			msgb_l2len(resp),
			sizeof(tOCTVC1_HW_MSG_CLOCK_SYNC_MGR_STATS_RSP),
			VTY_NEWLINE);
		return;
	}

	csr = (tOCTVC1_HW_MSG_CLOCK_SYNC_MGR_STATS_RSP *) msgb_l2(resp);

	vty_out(vty, "%s", VTY_NEWLINE);
	vty_out(vty, "CLOCK-SYNC-MGR-STATS:%s", VTY_NEWLINE);
	vty_out(vty, "State=%s%s",
		get_value_string(clocksync_state_vals, csr->ulState),
		VTY_NEWLINE);
	vty_out(vty, "ClockError=%d%s", csr->lClockError, VTY_NEWLINE);
	vty_out(vty, "DroppedCycles=%d%s", csr->lDroppedCycles, VTY_NEWLINE);
	vty_out(vty, "PllFreqHz=%u%s", csr->ulPllFreqHz, VTY_NEWLINE);
	vty_out(vty, "PllFract=%u%s", csr->ulPllFractionalFreqHz, VTY_NEWLINE);
	vty_out(vty, "SlipCnt=%u%s", csr->ulSlipCnt, VTY_NEWLINE);
	vty_out(vty, "SyncLosses=%u%s", csr->ulSyncLosseCnt, VTY_NEWLINE);
	vty_out(vty, "SourceState=%u%s", csr->ulSourceState, VTY_NEWLINE);
	vty_out(vty, "DacValue=%u%s", csr->ulDacValue, VTY_NEWLINE);
}

DEFUN(show_clk_sync_stats, show_clk_sync_stats_cmd,
	"show phy <0-255> clk-sync-stats",
	"Obtain statistics for the Clock Sync Manager\n")
{
	int phy_nr = atoi(argv[0]);
	struct phy_link *plink = phy_link_by_num(phy_nr);
	static struct octphy_hw_get_cb_data cb_data;

	cb_data.cb = show_clk_sync_stats_cb;
	cb_data.data = vty;

	octphy_hw_get_clock_sync_stats(plink->u.octphy.hdl, &cb_data);
	return CMD_SUCCESS;
}

void bts_model_config_write_phy(struct vty *vty, struct phy_link *plink)
{
	if (plink->u.octphy.netdev_name)
		vty_out(vty, " octphy net-device %s%s",
			plink->u.octphy.netdev_name, VTY_NEWLINE);

	vty_out(vty, " octphy hw-addr %02x:%02x:%02x:%02x:%02x:%02x%s",
		plink->u.octphy.phy_addr.sll_addr[0],
		plink->u.octphy.phy_addr.sll_addr[1],
		plink->u.octphy.phy_addr.sll_addr[2],
		plink->u.octphy.phy_addr.sll_addr[3],
		plink->u.octphy.phy_addr.sll_addr[4],
		plink->u.octphy.phy_addr.sll_addr[5],
		VTY_NEWLINE);
	vty_out(vty, " octphy rx-gain %u%s", plink->u.octphy.rx_gain_db,
		VTY_NEWLINE);

	if (plink->u.octphy.tx_atten_flag) {
		vty_out(vty, " octphy tx-attenuation %u%s",
			plink->u.octphy.tx_atten_db, VTY_NEWLINE);
	} else
		vty_out(vty, " octphy tx-attenuation oml%s", VTY_NEWLINE);

	vty_out(vty, " octphy rf-port-index %u%s", plink->u.octphy.rf_port_index,
		VTY_NEWLINE);
}

void bts_model_config_write_phy_inst(struct vty *vty, struct phy_instance *pinst)
{
}

void bts_model_config_write_bts(struct vty *vty, struct gsm_bts *bts)
{
}

void bts_model_config_write_trx(struct vty *vty, struct gsm_bts_trx *trx)
{
}

DEFUN(show_sys_info, show_sys_info_cmd,
	"show phy <0-255> system-information",
	SHOW_TRX_STR "Display information about system\n")
{
	int phy_nr = atoi(argv[0]);
	struct phy_link *plink = phy_link_by_num(phy_nr);
	struct octphy_hdl *fl1h;

	if (!plink) {
		vty_out(vty, "Cannot find PHY number %u%s",
			phy_nr, VTY_NEWLINE);
		return CMD_WARNING;
	}
	fl1h = plink->u.octphy.hdl;

	vty_out(vty, "System Platform: '%s', Version: '%s'%s",
		fl1h->info.system.platform, fl1h->info.system.version,
		VTY_NEWLINE);
	vty_out(vty, "Application Name: '%s', Description: '%s', Version: '%s'%s",
		fl1h->info.app.name, fl1h->info.app.description,
		fl1h->info.app.version, VTY_NEWLINE);

	return CMD_SUCCESS;
}


int bts_model_vty_init(struct gsm_bts *bts)
{
	vty_bts = bts;

	install_element(PHY_NODE, &cfg_phy_hwaddr_cmd);
	install_element(PHY_NODE, &cfg_phy_netdev_cmd);
	install_element(PHY_NODE, &cfg_phy_rf_port_idx_cmd);
	install_element(PHY_NODE, &cfg_phy_rx_gain_db_cmd);
	install_element(PHY_NODE, &cfg_phy_tx_atten_db_cmd);

	install_element_ve(&show_rf_port_stats_cmd);
	install_element_ve(&show_clk_sync_stats_cmd);
	install_element_ve(&show_sys_info_cmd);

	return 0;
}

int bts_model_ctrl_cmds_install(struct gsm_bts *bts)
{
	return 0;
}
