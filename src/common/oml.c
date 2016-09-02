/* GSM TS 12.21 O&M / OML, BTS side */

/* (C) 2011 by Andreas Eversberg <jolly@eversberg.eu>
 * (C) 2011-2013 by Harald Welte <laforge@gnumonks.org>
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
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
 * Operation and Maintainance Messages
 */

#include <errno.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include <osmocom/core/talloc.h>
#include <osmocom/gsm/protocol/gsm_12_21.h>
#include <osmocom/gsm/abis_nm.h>
#include <osmocom/abis/e1_input.h>
#include <osmocom/abis/ipaccess.h>

#include <osmo-bts/logging.h>
#include <osmo-bts/gsm_data.h>
#include <osmo-bts/abis.h>
#include <osmo-bts/oml.h>
#include <osmo-bts/bts_model.h>
#include <osmo-bts/bts.h>
#include <osmo-bts/signal.h>
#include <osmo-bts/pcu_if.h>
#include <osmo-bts/pcuif_proto.h>
#ifdef ENABLE_LC15BTS
#include "../osmo-bts-litecell15/lc15bts.h"
struct oml_fail_evt_rep_sig_data alarm_sig_data;
#endif

/* FIXME: move this to libosmocore */
static struct tlv_definition abis_nm_att_tlvdef_ipa = {
	.def = {
		/* ip.access specifics */
		[NM_ATT_IPACC_DST_IP] =		{ TLV_TYPE_FIXED, 4 },
		[NM_ATT_IPACC_DST_IP_PORT] =	{ TLV_TYPE_FIXED, 2 },
		[NM_ATT_IPACC_STREAM_ID] =	{ TLV_TYPE_TV, },
		[NM_ATT_IPACC_SEC_OML_CFG] =	{ TLV_TYPE_FIXED, 6 },
		[NM_ATT_IPACC_IP_IF_CFG] =	{ TLV_TYPE_FIXED, 8 },
		[NM_ATT_IPACC_IP_GW_CFG] =	{ TLV_TYPE_FIXED, 12 },
		[NM_ATT_IPACC_IN_SERV_TIME] =	{ TLV_TYPE_FIXED, 4 },
		[NM_ATT_IPACC_LOCATION] =	{ TLV_TYPE_TL16V },
		[NM_ATT_IPACC_PAGING_CFG] =	{ TLV_TYPE_FIXED, 2 },
		[NM_ATT_IPACC_UNIT_ID] =	{ TLV_TYPE_TL16V },
		[NM_ATT_IPACC_UNIT_NAME] =	{ TLV_TYPE_TL16V },
		[NM_ATT_IPACC_SNMP_CFG] =	{ TLV_TYPE_TL16V },
		[NM_ATT_IPACC_PRIM_OML_CFG_LIST] = { TLV_TYPE_TL16V },
		[NM_ATT_IPACC_NV_FLAGS] =	{ TLV_TYPE_TL16V },
		[NM_ATT_IPACC_FREQ_CTRL] =	{ TLV_TYPE_FIXED, 2 },
		[NM_ATT_IPACC_PRIM_OML_FB_TOUT] = { TLV_TYPE_TL16V },
		[NM_ATT_IPACC_CUR_SW_CFG] =	{ TLV_TYPE_TL16V },
		[NM_ATT_IPACC_TIMING_BUS] =	{ TLV_TYPE_TL16V },
		[NM_ATT_IPACC_CGI] =		{ TLV_TYPE_TL16V },
		[NM_ATT_IPACC_RAC] =		{ TLV_TYPE_TL16V },
		[NM_ATT_IPACC_OBJ_VERSION] =	{ TLV_TYPE_TL16V },
		[NM_ATT_IPACC_GPRS_PAGING_CFG]= { TLV_TYPE_TL16V },
		[NM_ATT_IPACC_NSEI] =		{ TLV_TYPE_TL16V },
		[NM_ATT_IPACC_BVCI] =		{ TLV_TYPE_TL16V },
		[NM_ATT_IPACC_NSVCI] =		{ TLV_TYPE_TL16V },
		[NM_ATT_IPACC_NS_CFG] =		{ TLV_TYPE_TL16V },
		[NM_ATT_IPACC_BSSGP_CFG] =	{ TLV_TYPE_TL16V },
		[NM_ATT_IPACC_NS_LINK_CFG] =	{ TLV_TYPE_TL16V },
		[NM_ATT_IPACC_RLC_CFG] =	{ TLV_TYPE_TL16V },
		[NM_ATT_IPACC_ALM_THRESH_LIST]=	{ TLV_TYPE_TL16V },
		[NM_ATT_IPACC_MONIT_VAL_LIST] = { TLV_TYPE_TL16V },
		[NM_ATT_IPACC_TIB_CONTROL] =	{ TLV_TYPE_TL16V },
		[NM_ATT_IPACC_SUPP_FEATURES] =	{ TLV_TYPE_TL16V },
		[NM_ATT_IPACC_CODING_SCHEMES] =	{ TLV_TYPE_TL16V },
		[NM_ATT_IPACC_RLC_CFG_2] =	{ TLV_TYPE_TL16V },
		[NM_ATT_IPACC_HEARTB_TOUT] =	{ TLV_TYPE_TL16V },
		[NM_ATT_IPACC_UPTIME] =		{ TLV_TYPE_TL16V },
		[NM_ATT_IPACC_RLC_CFG_3] =	{ TLV_TYPE_TL16V },
		[NM_ATT_IPACC_SSL_CFG] =	{ TLV_TYPE_TL16V },
		[NM_ATT_IPACC_SEC_POSSIBLE] =	{ TLV_TYPE_TL16V },
		[NM_ATT_IPACC_IML_SSL_STATE] =	{ TLV_TYPE_TL16V },
		[NM_ATT_IPACC_REVOC_DATE] =	{ TLV_TYPE_TL16V },
		/* GSM 12.21 attributes */
		[NM_ATT_MEAS_TYPE] = { TLV_TYPE_TV },
		[NM_ATT_MEAS_RES] = { TLV_TYPE_TV },
	},
};

static int oml_ipa_set_attr(struct gsm_bts *bts, struct msgb *msg);

/*
 * support
 */

struct tlv_parsed *tlvp_copy(const struct tlv_parsed *tp_orig, void *ctx)
{
	struct tlv_parsed *tp_out;
	unsigned int i;

	tp_out = talloc_zero(ctx, struct tlv_parsed);
	if (!tp_out)
		return NULL;

	/* if the original is NULL, return empty tlvp */
	if (!tp_orig)
		return tp_out;

	for (i = 0; i < ARRAY_SIZE(tp_orig->lv); i++) {
		unsigned int len = tp_orig->lv[i].len;
		tp_out->lv[i].len = len;
		if (len && tp_out->lv[i].val) {
			tp_out->lv[i].val = talloc_zero_size(tp_out, len);
			if (!tp_out->lv[i].val) {
				talloc_free(tp_out);
				return NULL;
			}
			memcpy((uint8_t *)tp_out->lv[i].val, tp_orig->lv[i].val, len);
		}
	}

	return tp_out;
}

/* merge all attributes of 'new' into 'out' */
int tlvp_merge(struct tlv_parsed *out, const struct tlv_parsed *new)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(out->lv); i++) {
		unsigned int len = new->lv[i].len;
		if (len == 0 || new->lv[i].val == NULL)
			continue;
		if (out->lv[i].val) {
			talloc_free((uint8_t *) out->lv[i].val);
			out->lv[i].len = 0;
		}
		out->lv[i].val = talloc_zero_size(out, len);
		if (!out->lv[i].val)
			return -ENOMEM;
		memcpy((uint8_t *) out->lv[i].val, new->lv[i].val, len);
	}
	return 0;
}

static int oml_tlv_parse(struct tlv_parsed *tp, const uint8_t *buf, int len)
{
	return tlv_parse(tp, &abis_nm_att_tlvdef_ipa, buf, len, 0, 0);
}

struct msgb *oml_msgb_alloc(void)
{
	return msgb_alloc_headroom(1024, 128, "OML");
}

int oml_send_msg(struct msgb *msg, int is_manuf)
{
	struct abis_om_hdr *omh;

	if (is_manuf) {
		/* length byte, string + 0 termination */
		uint8_t *manuf = msgb_push(msg, 1 + sizeof(abis_nm_ipa_magic));
		manuf[0] = strlen(abis_nm_ipa_magic)+1;
		memcpy(manuf+1, abis_nm_ipa_magic, strlen(abis_nm_ipa_magic));
	}

	/* Push the main OML header and send it off */
	omh = (struct abis_om_hdr *) msgb_push(msg, sizeof(*omh));
	if (is_manuf)
		omh->mdisc = ABIS_OM_MDISC_MANUF;
	else
		omh->mdisc = ABIS_OM_MDISC_FOM;
	omh->placement = ABIS_OM_PLACEMENT_ONLY;
	omh->sequence = 0;
	omh->length = msgb_l3len(msg);

	msg->l2h = (uint8_t *)omh;

	return abis_oml_sendmsg(msg);
}

int oml_mo_send_msg(struct gsm_abis_mo *mo, struct msgb *msg, uint8_t msg_type)
{
	struct abis_om_fom_hdr *foh;

	msg->l3h = msgb_push(msg, sizeof(*foh));
	foh = (struct abis_om_fom_hdr *) msg->l3h;
	foh->msg_type = msg_type;
	foh->obj_class = mo->obj_class;
	memcpy(&foh->obj_inst, &mo->obj_inst, sizeof(foh->obj_inst));

	/* FIXME: This assumption may not always be correct */
	msg->trx = mo->bts->c0;

	return oml_send_msg(msg, 0);
}

/* FIXME: move to gsm_data_shared */
static char mo_buf[128];
char *gsm_abis_mo_name(const struct gsm_abis_mo *mo)
{
	snprintf(mo_buf, sizeof(mo_buf), "OC=%s INST=(%02x,%02x,%02x)",
		 get_value_string(abis_nm_obj_class_names, mo->obj_class),
		 mo->obj_inst.bts_nr, mo->obj_inst.trx_nr, mo->obj_inst.ts_nr);
	return mo_buf;
}

/* 8.8.1 sending State Changed Event Report */
int oml_tx_state_changed(struct gsm_abis_mo *mo)
{
	struct msgb *nmsg;

	LOGP(DOML, LOGL_INFO, "%s Tx STATE CHG REP\n", gsm_abis_mo_name(mo));

	nmsg = oml_msgb_alloc();
	if (!nmsg)
		return -ENOMEM;

	/* 9.4.38 Operational State */
	msgb_tv_put(nmsg, NM_ATT_OPER_STATE, mo->nm_state.operational);

	/* 9.4.7 Availability Status */
	msgb_tl16v_put(nmsg, NM_ATT_AVAIL_STATUS, 1, &mo->nm_state.availability);

	return oml_mo_send_msg(mo, nmsg, NM_MT_STATECHG_EVENT_REP);
}

/* First initialization of MO, does _not_ generate state changes */
void oml_mo_state_init(struct gsm_abis_mo *mo, int op_state, int avail_state)
{
	mo->nm_state.availability = avail_state;
	mo->nm_state.operational = op_state;
}

int oml_mo_state_chg(struct gsm_abis_mo *mo, int op_state, int avail_state)
{
	int rc = 0;

	if ((op_state != -1 && mo->nm_state.operational != op_state) ||
	    (avail_state != -1 && mo->nm_state.availability != avail_state)) {
		if (avail_state != -1) {
			LOGP(DOML, LOGL_INFO, "%s AVAIL STATE %s -> %s\n",
				gsm_abis_mo_name(mo),
				abis_nm_avail_name(mo->nm_state.availability),
				abis_nm_avail_name(avail_state));
			mo->nm_state.availability = avail_state;
		}
		if (op_state != -1) {
			LOGP(DOML, LOGL_INFO, "%s OPER STATE %s -> %s\n",
				gsm_abis_mo_name(mo),
				abis_nm_opstate_name(mo->nm_state.operational),
				abis_nm_opstate_name(op_state));
			mo->nm_state.operational = op_state;
			osmo_signal_dispatch(SS_GLOBAL, S_NEW_OP_STATE, NULL);
		}

		/* send state change report */
		rc = oml_tx_state_changed(mo);
	}
	return rc;
}

int oml_mo_fom_ack_nack(struct gsm_abis_mo *mo, uint8_t orig_msg_type,
			uint8_t cause)
{
	struct msgb *msg;
	uint8_t new_msg_type;

	msg = oml_msgb_alloc();
	if (!msg)
		return -ENOMEM;

	if (cause) {
		new_msg_type = orig_msg_type + 2;
		msgb_tv_put(msg, NM_ATT_NACK_CAUSES, cause);
	} else {
		new_msg_type = orig_msg_type + 1;
	}

	return oml_mo_send_msg(mo, msg, new_msg_type);
}

int oml_mo_statechg_ack(struct gsm_abis_mo *mo)
{
	struct msgb *msg;

	msg = oml_msgb_alloc();
	if (!msg)
		return -ENOMEM;

	msgb_tv_put(msg, NM_ATT_ADM_STATE, mo->nm_state.administrative);

	return oml_mo_send_msg(mo, msg, NM_MT_CHG_ADM_STATE_ACK);
}

int oml_mo_statechg_nack(struct gsm_abis_mo *mo, uint8_t nack_cause)
{
	return oml_mo_fom_ack_nack(mo, NM_MT_CHG_ADM_STATE, nack_cause);
}

int oml_mo_opstart_ack(struct gsm_abis_mo *mo)
{
	return oml_mo_fom_ack_nack(mo, NM_MT_OPSTART, 0);
}

int oml_mo_opstart_nack(struct gsm_abis_mo *mo, uint8_t nack_cause)
{
	return oml_mo_fom_ack_nack(mo, NM_MT_OPSTART, nack_cause);
}

int oml_fom_ack_nack(struct msgb *old_msg, uint8_t cause)
{
	struct abis_om_hdr *old_oh = msgb_l2(old_msg);
	struct abis_om_fom_hdr *old_foh = msgb_l3(old_msg);
	struct msgb *msg;
	struct abis_om_fom_hdr *foh;
	int is_manuf = 0;

	msg = oml_msgb_alloc();
	if (!msg)
		return -ENOMEM;

	/* make sure to respond with MANUF if request was MANUF */
	if (old_oh->mdisc == ABIS_OM_MDISC_MANUF)
		is_manuf = 1;

	msg->trx = old_msg->trx;

	/* copy over old FOM-Header and later only change the msg_type */
	msg->l3h = msgb_push(msg, sizeof(*foh));
	foh = (struct abis_om_fom_hdr *) msg->l3h;
	memcpy(foh, old_foh, sizeof(*foh));

	/* alter message type */
	if (cause) {
		LOGP(DOML, LOGL_NOTICE, "Sending FOM NACK with cause %s.\n",
			abis_nm_nack_cause_name(cause));
		foh->msg_type += 2; /* nack */
		/* add cause */
		msgb_tv_put(msg, NM_ATT_NACK_CAUSES, cause);
	} else {
		LOGP(DOML, LOGL_DEBUG, "Sending FOM ACK.\n");
		foh->msg_type++; /* ack */
	}

	return oml_send_msg(msg, is_manuf);
}

/*
 * Formatted O&M messages
 */

/* 8.3.7 sending SW Activated Report */
int oml_mo_tx_sw_act_rep(struct gsm_abis_mo *mo)
{
	struct msgb *nmsg;

	LOGP(DOML, LOGL_INFO, "%s Tx SW ACT REP\n", gsm_abis_mo_name(mo));

	nmsg = oml_msgb_alloc();
	if (!nmsg)
		return -ENOMEM;

	msgb_put(nmsg, sizeof(struct abis_om_fom_hdr));
	return oml_mo_send_msg(mo, nmsg, NM_MT_SW_ACTIVATED_REP);
}

/* TS 12.21 8.8.2 */
int oml_tx_nm_fail_evt_rep(struct gsm_abis_mo *mo, uint8_t event_type, uint8_t event_serverity, uint8_t cause_type, uint16_t event_cause, char *add_text)
{
	struct msgb *nmsg;
	uint8_t cause[3];
	int i, len;

	LOGP(DOML, LOGL_INFO, "%s Tx FAILure EVT REP\n", gsm_abis_mo_name(mo));

	nmsg = oml_msgb_alloc();
	if (!nmsg)
		return -ENOMEM;

	msgb_tv_put(nmsg, NM_ATT_EVENT_TYPE, event_type);
	msgb_tv_put(nmsg, NM_ATT_SEVERITY, event_serverity);

	cause[0] = cause_type;
	for (i = 0; i < 2 ; i++)
		cause[i + 1] = ((uint8_t*)&event_cause)[1 - i];

	msgb_tv_fixed_put(nmsg, NM_ATT_PROB_CAUSE, 3, cause);

	len = strlen(add_text);
	if(len){
		LOGP(DOML, LOGL_DEBUG, "%s Tx FAILure EVT REP Additional Text = %s (%d)\n", gsm_abis_mo_name(mo), add_text, len);
		msgb_tl16v_put(nmsg, NM_ATT_ADD_TEXT, len, add_text);
	}

	return oml_mo_send_msg(mo, nmsg, NM_MT_FAILURE_EVENT_REP);
}

/* TS 12.21 8.10.2 */
int oml_tx_nm_meas_res_resp(struct gsm_abis_mo *mo, struct gsm_pcu_if_meas_resp meas_resp)
{
	struct msgb *nmsg;

	LOGP(DOML, LOGL_INFO, "%s Tx MEASurement RESult RESPonse\n", gsm_abis_mo_name(mo));

	nmsg = oml_msgb_alloc();
	if (!nmsg)
		return -ENOMEM;

	msgb_tv_put(nmsg, NM_ATT_MEAS_TYPE, meas_resp.meas_id);
	msgb_tl16v_put(nmsg, NM_ATT_MEAS_RES, meas_resp.len, meas_resp.data);

	return oml_mo_send_msg(mo, nmsg, NM_MT_MEAS_RES_RESP);
}

int oml_tx_nm_start_meas_ack_nack(struct gsm_abis_mo *mo, uint8_t meas_id, uint8_t nack_cause)
{
	struct msgb *msg;
	uint8_t msg_type;

	msg = oml_msgb_alloc();
	if (!msg)
		return -ENOMEM;

	if (nack_cause) {
		msg_type =  NM_MT_IPACC_START_MEAS_NACK;
		msgb_tv_put(msg, NM_ATT_NACK_CAUSES, nack_cause);
		msgb_tv_put(msg, NM_ATT_MEAS_TYPE, meas_id);
	} else {
		msg_type = NM_MT_IPACC_START_MEAS_ACK;
		msgb_tv_put(msg, NM_ATT_MEAS_TYPE, meas_id);
	}

	return oml_mo_send_msg(mo, msg, msg_type);
}

int oml_tx_nm_stop_meas_ack_nack(struct gsm_abis_mo *mo, uint8_t meas_id, uint8_t nack_cause)
{
	struct msgb *msg;
	uint8_t msg_type;

	msg = oml_msgb_alloc();
	if (!msg)
		return -ENOMEM;

	if (nack_cause) {
		msg_type =  NM_MT_IPACC_STOP_MEAS_NACK;
		msgb_tv_put(msg, NM_ATT_NACK_CAUSES, nack_cause);
		msgb_tv_put(msg, NM_ATT_MEAS_TYPE, meas_id);
	} else {
		msg_type = NM_MT_IPACC_STOP_MEAS_ACK;
		msgb_tv_put(msg, NM_ATT_MEAS_TYPE, meas_id);
	}

	return oml_mo_send_msg(mo, msg, msg_type);
}

int oml_tx_nm_meas_res_req_nack(struct gsm_abis_mo *mo, uint8_t meas_id, uint8_t nack_cause)
{
	struct msgb *msg;
	uint8_t msg_type;

	msg = oml_msgb_alloc();
	if (!msg)
		return -ENOMEM;

	msg_type =  NM_MT_IPACC_MEAS_RES_REQ_NACK;
	msgb_tv_put(msg, NM_ATT_NACK_CAUSES, nack_cause);
	msgb_tv_put(msg, NM_ATT_MEAS_TYPE, meas_id);

	return oml_mo_send_msg(mo, msg, msg_type);
}

/* TS 12.21 9.4.53 */
enum abis_nm_t200_idx {
	T200_SDCCH		= 0,
	T200_FACCH_F		= 1,
	T200_FACCH_H		= 2,
	T200_SACCH_TCH_SAPI0	= 3,
	T200_SACCH_SDCCH	= 4,
	T200_SDCCH_SAPI3	= 5,
	T200_SACCH_TCH_SAPI3	= 6
};

/* TS 12.21 9.4.53 */
static const uint8_t abis_nm_t200_mult[] = {
	[T200_SDCCH]		= 5,
	[T200_FACCH_F]		= 5,
	[T200_FACCH_H]		= 5,
	[T200_SACCH_TCH_SAPI0]	= 10,
	[T200_SACCH_SDCCH]	= 10,
	[T200_SDCCH_SAPI3]	= 5,
	[T200_SACCH_TCH_SAPI3]	= 10
};

/* the below defaults correpsond to the libocmocore default of 1s for
 * DCCH and 2s for ACCH. The BSC should overried this via OML anyway. */
const unsigned int oml_default_t200_ms[7] = {
	[T200_SDCCH]		= 1000,
	[T200_FACCH_F]		= 1000,
	[T200_FACCH_H]		= 1000,
	[T200_SACCH_TCH_SAPI0]	= 2000,
	[T200_SACCH_SDCCH]	= 2000,
	[T200_SDCCH_SAPI3]	= 1000,
	[T200_SACCH_TCH_SAPI3]	= 2000,
};

static void dl_set_t200(struct lapdm_datalink *dl, unsigned int t200_msec)
{
	dl->dl.t200_sec = t200_msec / 1000;
	dl->dl.t200_usec = (t200_msec % 1000) * 1000;
}

/* Configure LAPDm T200 timers for this lchan according to OML */
int oml_set_lchan_t200(struct gsm_lchan *lchan)
{
	struct gsm_bts *bts = lchan->ts->trx->bts;
	struct gsm_bts_role_bts *btsb = bts_role_bts(bts);
	struct lapdm_channel *lc = &lchan->lapdm_ch;
	unsigned int t200_dcch, t200_dcch_sapi3, t200_acch, t200_acch_sapi3;

	/* set T200 for main and associated channel */
	switch (lchan->type) {
	case GSM_LCHAN_SDCCH:
		t200_dcch = btsb->t200_ms[T200_SDCCH];
		t200_dcch_sapi3 = btsb->t200_ms[T200_SDCCH_SAPI3];
		t200_acch = btsb->t200_ms[T200_SACCH_SDCCH];
		t200_acch_sapi3 = btsb->t200_ms[T200_SACCH_SDCCH];
		break;
	case GSM_LCHAN_TCH_F:
		t200_dcch = btsb->t200_ms[T200_FACCH_F];
		t200_dcch_sapi3 = btsb->t200_ms[T200_FACCH_F];
		t200_acch = btsb->t200_ms[T200_SACCH_TCH_SAPI0];
		t200_acch_sapi3 = btsb->t200_ms[T200_SACCH_TCH_SAPI3];
		break;
	case GSM_LCHAN_TCH_H:
		t200_dcch = btsb->t200_ms[T200_FACCH_H];
		t200_dcch_sapi3 = btsb->t200_ms[T200_FACCH_H];
		t200_acch = btsb->t200_ms[T200_SACCH_TCH_SAPI0];
		t200_acch_sapi3 = btsb->t200_ms[T200_SACCH_TCH_SAPI3];
		break;
	default:
		return -1;
	}

	DEBUGP(DLLAPD, "%s: Setting T200 D0=%u, D3=%u, S0=%u, S3=%u"
		"(all in ms)\n", gsm_lchan_name(lchan), t200_dcch,
		t200_dcch_sapi3, t200_acch, t200_acch_sapi3);

	dl_set_t200(&lc->lapdm_dcch.datalink[DL_SAPI0], t200_dcch);
	dl_set_t200(&lc->lapdm_dcch.datalink[DL_SAPI3], t200_dcch_sapi3);
	dl_set_t200(&lc->lapdm_acch.datalink[DL_SAPI0], t200_acch);
	dl_set_t200(&lc->lapdm_acch.datalink[DL_SAPI3], t200_acch_sapi3);

	return 0;
}

/* 8.6.1 Set BTS Attributes has been received */
static int oml_rx_set_bts_attr(struct gsm_bts *bts, struct msgb *msg)
{
	struct abis_om_fom_hdr *foh = msgb_l3(msg);
	struct tlv_parsed tp, *tp_merged;
	struct gsm_bts_role_bts *btsb = bts_role_bts(bts);
	int rc, i;
	const uint8_t *payload;

	abis_nm_debugp_foh(DOML, foh);
	DEBUGPC(DOML, "Rx SET BTS ATTR\n");

	rc = oml_tlv_parse(&tp, foh->data, msgb_l3len(msg) - sizeof(*foh));
	if (rc < 0)
		return oml_fom_ack_nack(msg, NM_NACK_INCORR_STRUCT);

	/* Test for globally unsupported stuff here */
	if (TLVP_PRESENT(&tp, NM_ATT_BCCH_ARFCN)) {
		uint16_t arfcn = ntohs(tlvp_val16_unal(&tp, NM_ATT_BCCH_ARFCN));
		if (arfcn > 1024) {
			LOGP(DOML, LOGL_NOTICE, "Given ARFCN %d is not supported.\n", arfcn);
			return oml_fom_ack_nack(msg, NM_NACK_FREQ_NOTAVAIL);
		}
	}
	/* 9.4.52 Starting Time */
	if (TLVP_PRESENT(&tp, NM_ATT_START_TIME)) {
		return oml_fom_ack_nack(msg, NM_NACK_SPEC_IMPL_NOTSUPP);
	}

	/* merge existing BTS attributes with new attributes */
	tp_merged = tlvp_copy(bts->mo.nm_attr, bts);
	tlvp_merge(tp_merged, &tp);

	/* Ask BTS driver to validate new merged attributes */
	rc = bts_model_check_oml(bts, foh->msg_type, bts->mo.nm_attr, tp_merged, bts);
	if (rc < 0) {
		talloc_free(tp_merged);
		return oml_fom_ack_nack(msg, -rc);
	}

	/* Success: replace old BTS attributes with new */
	talloc_free(bts->mo.nm_attr);
	bts->mo.nm_attr = tp_merged;

	/* ... and actually still parse them */

	/* 9.4.25 Interference Level Boundaries */
	if (TLVP_PRESENT(&tp, NM_ATT_INTERF_BOUND)) {
		payload = TLVP_VAL(&tp, NM_ATT_INTERF_BOUND);
		for (i = 0; i < 6; i++) {
			int16_t boundary = *payload;
			btsb->interference.boundary[i] = -1 * boundary;
		}
	}
	/* 9.4.24 Intave Parameter */
	if (TLVP_PRESENT(&tp, NM_ATT_INTAVE_PARAM))
		btsb->interference.intave = *TLVP_VAL(&tp, NM_ATT_INTAVE_PARAM);

	/* 9.4.14 Connection Failure Criterion */
	if (TLVP_PRESENT(&tp, NM_ATT_CONN_FAIL_CRIT)) {
		const uint8_t *val = TLVP_VAL(&tp, NM_ATT_CONN_FAIL_CRIT);

		if (TLVP_LEN(&tp, NM_ATT_CONN_FAIL_CRIT) < 2
		 || val[0] != 0x01 || val[1] < 4 || val[1] > 64) {
			LOGP(DOML, LOGL_NOTICE, "Given Conn. Failure Criterion "
				"not supported. Please use critetion 0x01 with "
				"RADIO_LINK_TIMEOUT value of 4..64\n");
			return oml_fom_ack_nack(msg, NM_NACK_PARAM_RANGE);
		}
		btsb->radio_link_timeout = val[1];
	}
	/* if val[0] != 0x01: can be 'operator dependent' and needs to
	 * be parsed by bts driver */

	/* 9.4.53 T200 */
	if (TLVP_PRESENT(&tp, NM_ATT_T200)) {
		payload = TLVP_VAL(&tp, NM_ATT_T200);
		for (i = 0; i < ARRAY_SIZE(btsb->t200_ms); i++) {
			uint32_t t200_ms = payload[i] * abis_nm_t200_mult[i];
#if 0
			btsb->t200_ms[i] = t200_ms;
			DEBUGP(DOML, "T200[%u]: OML=%u, mult=%u => %u ms\n",
				i, payload[i], abis_nm_t200_mult[i],
				btsb->t200_ms[i]);
#else
			/* we'd rather use the 1s/2s (long) defaults by
			 * libosmocore, as we appear to have some bug(s)
			 * related to handling T200 expiration in
			 * libosmogsm lapd(m) code? */
			LOGP(DOML, LOGL_NOTICE, "Ignoring T200[%u] (%u ms) "
				"as sent by BSC due to suspected LAPDm bug!\n",
				i, t200_ms);
#endif
		}
	}

	/* 9.4.31 Maximum Timing Advance */
	if (TLVP_PRESENT(&tp, NM_ATT_MAX_TA))
		btsb->max_ta = *TLVP_VAL(&tp, NM_ATT_MAX_TA);

	/* 9.4.39 Overload Period */
	if (TLVP_PRESENT(&tp, NM_ATT_OVERL_PERIOD))
		btsb->load.overload_period = *TLVP_VAL(&tp, NM_ATT_OVERL_PERIOD);

	/* 9.4.12 CCCH Load Threshold */
	if (TLVP_PRESENT(&tp, NM_ATT_CCCH_L_T))
		btsb->load.ccch.load_ind_thresh = *TLVP_VAL(&tp, NM_ATT_CCCH_L_T);

	/* 9.4.11 CCCH Load Indication Period */
	if (TLVP_PRESENT(&tp, NM_ATT_CCCH_L_I_P))
		btsb->load.ccch.load_ind_period = *TLVP_VAL(&tp, NM_ATT_CCCH_L_I_P);

	/* 9.4.44 RACH Busy Threshold */
	if (TLVP_PRESENT(&tp, NM_ATT_RACH_B_THRESH)) {
		int16_t thresh = *TLVP_VAL(&tp, NM_ATT_RACH_B_THRESH);
		btsb->load.rach.busy_thresh = -1 * thresh;
	}

	/* 9.4.45 RACH Load Averaging Slots */
	if (TLVP_PRESENT(&tp, NM_ATT_LDAVG_SLOTS)) {
		btsb->load.rach.averaging_slots =
			ntohs(tlvp_val16_unal(&tp, NM_ATT_LDAVG_SLOTS));
	}

	/* 9.4.10 BTS Air Timer */
	if (TLVP_PRESENT(&tp, NM_ATT_BTS_AIR_TIMER)) {
		uint8_t t3105 = *TLVP_VAL(&tp, NM_ATT_BTS_AIR_TIMER);
		if (t3105 == 0) {
			LOGP(DOML, LOGL_NOTICE,
				"T3105 must have a value != 0.\n");
			return oml_fom_ack_nack(msg, NM_NACK_PARAM_RANGE);
		}
		btsb->t3105_ms = t3105 * 10;
	}

	/* 9.4.37 NY1 */
	if (TLVP_PRESENT(&tp, NM_ATT_NY1))
		btsb->ny1 = *TLVP_VAL(&tp, NM_ATT_NY1);
	
	/* 9.4.8 BCCH ARFCN */
	if (TLVP_PRESENT(&tp, NM_ATT_BCCH_ARFCN))
		bts->c0->arfcn = ntohs(tlvp_val16_unal(&tp, NM_ATT_BCCH_ARFCN));

	/* 9.4.9 BSIC */
	if (TLVP_PRESENT(&tp, NM_ATT_BSIC))
		bts->bsic = *TLVP_VAL(&tp, NM_ATT_BSIC);

	/* call into BTS driver to apply new attributes to hardware */
	return bts_model_apply_oml(bts, msg, tp_merged, NM_OC_BTS, bts);
}

/* 8.6.2 Set Radio Attributes has been received */
static int oml_rx_set_radio_attr(struct gsm_bts_trx *trx, struct msgb *msg)
{
	struct abis_om_fom_hdr *foh = msgb_l3(msg);
	struct tlv_parsed tp, *tp_merged;
	int rc;

	abis_nm_debugp_foh(DOML, foh);
	DEBUGPC(DOML, "Rx SET RADIO CARRIER ATTR\n");

	rc = oml_tlv_parse(&tp, foh->data, msgb_l3len(msg) - sizeof(*foh));
	if (rc < 0)
		return oml_fom_ack_nack(msg, NM_NACK_INCORR_STRUCT);

	/* merge existing BTS attributes with new attributes */
	tp_merged = tlvp_copy(trx->mo.nm_attr, trx->bts);
	tlvp_merge(tp_merged, &tp);

	/* Ask BTS driver to validate new merged attributes */
	rc = bts_model_check_oml(trx->bts, foh->msg_type, trx->mo.nm_attr, tp_merged, trx);
	if (rc < 0) {
		talloc_free(tp_merged);
		return oml_fom_ack_nack(msg, -rc);
	}

	/* Success: replace old BTS attributes with new */
	talloc_free(trx->mo.nm_attr);
	trx->mo.nm_attr = tp_merged;

	/* ... and actually still parse them */

	/* 9.4.47 RF Max Power Reduction */
	if (TLVP_PRESENT(&tp, NM_ATT_RF_MAXPOWR_R)) {
		trx->max_power_red = *TLVP_VAL(&tp, NM_ATT_RF_MAXPOWR_R) * 2;
		LOGP(DOML, LOGL_INFO, "Set RF Max Power Reduction = %d dBm\n",
		     trx->max_power_red);
	}
	/* 9.4.5 ARFCN List */
#if 0
	if (TLVP_PRESENT(&tp, NM_ATT_ARFCN_LIST)) {
		uint8_t *value = TLVP_VAL(&tp, NM_ATT_ARFCN_LIST);
		uint16_t _value;
		uint16_t length = TLVP_LEN(&tp, NM_ATT_ARFCN_LIST);
		uint16_t arfcn;
		int i;
		for (i = 0; i < length; i++) {
			memcpy(&_value, value, 2);
			arfcn = ntohs(_value);
			value += 2;
			if (arfcn > 1024)
				return oml_fom_ack_nack(msg, NM_NACK_FREQ_NOTAVAIL);
			trx->arfcn_list[i] = arfcn;
			LOGP(DOML, LOGL_INFO, " ARFCN list = %d\n", trx->arfcn_list[i]);
		}
		trx->arfcn_num = length;
	} else
		trx->arfcn_num = 0;
#else
	if (trx != trx->bts->c0 && TLVP_PRESENT(&tp, NM_ATT_ARFCN_LIST)) {
		const uint8_t *value = TLVP_VAL(&tp, NM_ATT_ARFCN_LIST);
		uint16_t _value;
		uint16_t length = TLVP_LEN(&tp, NM_ATT_ARFCN_LIST);
		uint16_t arfcn;
		if (length != 2) {
			LOGP(DOML, LOGL_ERROR, "Expecting only one ARFCN, "
				"because hopping not supported\n");
			/* FIXME: send NACK */
			return -ENOTSUP;
		}
		memcpy(&_value, value, 2);
		arfcn = ntohs(_value);
		value += 2;
		if (arfcn > 1024)
			return oml_fom_ack_nack(msg, NM_NACK_FREQ_NOTAVAIL);
		trx->arfcn = arfcn;
	}
#endif
	/* call into BTS driver to apply new attributes to hardware */
	return bts_model_apply_oml(trx->bts, msg, tp_merged, NM_OC_RADIO_CARRIER, trx);
}

static int conf_lchans(struct gsm_bts_trx_ts *ts)
{
	enum gsm_phys_chan_config pchan = ts->pchan;

	/* RSL_MT_IPAC_PDCH_ACT style dyn PDCH */
	if (pchan == GSM_PCHAN_TCH_F_PDCH)
		pchan = ts->flags & TS_F_PDCH_ACTIVE? GSM_PCHAN_PDCH
						    : GSM_PCHAN_TCH_F;

	/* Osmocom RSL CHAN ACT style dyn TS */
	if (pchan == GSM_PCHAN_TCH_F_TCH_H_PDCH) {
		pchan = ts->dyn.pchan_is;

		/* If the dyn TS doesn't have a pchan yet, do nothing. */
		if (pchan == GSM_PCHAN_NONE)
			return 0;
	}

	return conf_lchans_as_pchan(ts, pchan);
}

int conf_lchans_as_pchan(struct gsm_bts_trx_ts *ts,
			 enum gsm_phys_chan_config pchan)
{
	struct gsm_lchan *lchan;
	unsigned int i;

	switch (pchan) {
	case GSM_PCHAN_CCCH_SDCCH4_CBCH:
		/* fallthrough */
	case GSM_PCHAN_CCCH_SDCCH4:
		for (i = 0; i < 4; i++) {
			lchan = &ts->lchan[i];
			if (pchan == GSM_PCHAN_CCCH_SDCCH4_CBCH
			    && i == 2) {
				lchan->type = GSM_LCHAN_CBCH;
			} else {
				lchan->type = GSM_LCHAN_SDCCH;
			}
		}
		/* fallthrough */
	case GSM_PCHAN_CCCH:
		lchan = &ts->lchan[4];
		lchan->type = GSM_LCHAN_CCCH;
		break;
	case GSM_PCHAN_TCH_F:
		lchan = &ts->lchan[0];
		lchan->type = GSM_LCHAN_TCH_F;
		break;
	case GSM_PCHAN_TCH_H:
		for (i = 0; i < 2; i++) {
			lchan = &ts->lchan[i];
			lchan->type = GSM_LCHAN_TCH_H;
		}
		break;
	case GSM_PCHAN_SDCCH8_SACCH8C_CBCH:
		/* fallthrough */
	case GSM_PCHAN_SDCCH8_SACCH8C:
		for (i = 0; i < 8; i++) {
			lchan = &ts->lchan[i];
			if (pchan == GSM_PCHAN_SDCCH8_SACCH8C_CBCH
			    && i == 2) {
				lchan->type = GSM_LCHAN_CBCH;
			} else {
				lchan->type = GSM_LCHAN_SDCCH;
			}
		}
		break;
	case GSM_PCHAN_PDCH:
		lchan = &ts->lchan[0];
		lchan->type = GSM_LCHAN_PDTCH;
		break;
	default:
		LOGP(DOML, LOGL_ERROR, "Unknown/unhandled PCHAN type: %u %s\n",
		     ts->pchan, gsm_pchan_name(ts->pchan));
		return -NM_NACK_PARAM_RANGE;
	}
	return 0;
}

/* 8.6.3 Set Channel Attributes has been received */
static int oml_rx_set_chan_attr(struct gsm_bts_trx_ts *ts, struct msgb *msg)
{
	struct abis_om_fom_hdr *foh = msgb_l3(msg);
	struct gsm_bts *bts = ts->trx->bts;
	struct tlv_parsed tp, *tp_merged;
	int rc;

	abis_nm_debugp_foh(DOML, foh);
	DEBUGPC(DOML, "Rx SET CHAN ATTR\n");

	rc = oml_tlv_parse(&tp, foh->data, msgb_l3len(msg) - sizeof(*foh));
	if (rc < 0)
		return oml_fom_ack_nack(msg, NM_NACK_INCORR_STRUCT);

	/* 9.4.21 HSN... */
	/* 9.4.27 MAIO */
	if (TLVP_PRESENT(&tp, NM_ATT_HSN) || TLVP_PRESENT(&tp, NM_ATT_MAIO)) {
		LOGP(DOML, LOGL_NOTICE, "SET CHAN ATTR: Frequency hopping not supported.\n");
		return oml_fom_ack_nack(msg, NM_NACK_SPEC_IMPL_NOTSUPP);
	}

	/* 9.4.52 Starting Time */
	if (TLVP_PRESENT(&tp, NM_ATT_START_TIME)) {
		LOGP(DOML, LOGL_NOTICE, "SET CHAN ATTR: Starting time not supported.\n");
		return oml_fom_ack_nack(msg, NM_NACK_SPEC_IMPL_NOTSUPP);
	}

	/* merge existing BTS attributes with new attributes */
	tp_merged = tlvp_copy(ts->mo.nm_attr, bts);
	tlvp_merge(tp_merged, &tp);

	/* Call into BTS driver to check attribute values */
	rc = bts_model_check_oml(bts, foh->msg_type, ts->mo.nm_attr, tp_merged, ts);
	if (rc < 0) {
		talloc_free(tp_merged);
		/* Send NACK */
		return oml_fom_ack_nack(msg, -rc);
	}

	/* Success: replace old BTS attributes with new */
	talloc_free(ts->mo.nm_attr);
	ts->mo.nm_attr = tp_merged;

	/* 9.4.13 Channel Combination */
	if (TLVP_PRESENT(&tp, NM_ATT_CHAN_COMB)) {
		uint8_t comb = *TLVP_VAL(&tp, NM_ATT_CHAN_COMB);
		ts->pchan = abis_nm_pchan4chcomb(comb);
		rc = conf_lchans(ts);
		if (rc < 0) {
			talloc_free(tp_merged);
			/* Send NACK */
			return oml_fom_ack_nack(msg, -rc);
		}
	}

	/* 9.4.5 ARFCN List */

	/* 9.4.60 TSC */
	if (TLVP_PRESENT(&tp, NM_ATT_TSC) && TLVP_LEN(&tp, NM_ATT_TSC) >= 1) {
		ts->tsc = *TLVP_VAL(&tp, NM_ATT_TSC);
	} else {
		/* If there is no TSC specified, use the BCC */
		ts->tsc = bts->bsic & 0x3;
	}
	LOGP(DOML, LOGL_INFO, "%s SET CHAN ATTR (TSC = %u)\n",
		gsm_abis_mo_name(&ts->mo), ts->tsc);

	/* call into BTS driver to apply new attributes to hardware */
	return bts_model_apply_oml(bts, msg, tp_merged, NM_OC_CHANNEL, ts);
}

/* 8.9.2 Opstart has been received */
static int oml_rx_opstart(struct gsm_bts *bts, struct msgb *msg)
{
	struct abis_om_fom_hdr *foh = msgb_l3(msg);
	struct gsm_abis_mo *mo;
	void *obj;

	abis_nm_debugp_foh(DOML, foh);
	DEBUGPC(DOML, "Rx OPSTART\n");

	/* Step 1: Resolve MO by obj_class/obj_inst */
	mo = gsm_objclass2mo(bts, foh->obj_class, &foh->obj_inst);
	obj = gsm_objclass2obj(bts, foh->obj_class, &foh->obj_inst);
	if (!mo || !obj)
		return oml_fom_ack_nack(msg, NM_NACK_OBJINST_UNKN);

	/* Step 2: Do some global dependency/consistency checking */
	if (mo->nm_state.operational == NM_OPSTATE_ENABLED) {
		DEBUGP(DOML, "... automatic ACK, OP state already was Enabled\n");
		return oml_mo_opstart_ack(mo);
	}

	/* Step 3: Ask BTS driver to apply the opstart */
	return bts_model_opstart(bts, mo, obj);
}

static int oml_rx_chg_adm_state(struct gsm_bts *bts, struct msgb *msg)
{
	struct abis_om_fom_hdr *foh = msgb_l3(msg);
	struct tlv_parsed tp;
	struct gsm_abis_mo *mo;
	uint8_t adm_state;
	void *obj;
	int rc;

	abis_nm_debugp_foh(DOML, foh);
	DEBUGPC(DOML, "Rx CHG ADM STATE\n");

	rc = oml_tlv_parse(&tp, foh->data, msgb_l3len(msg) - sizeof(*foh));
	if (rc < 0) {
		LOGP(DOML, LOGL_ERROR, "Rx CHG ADM STATE: error during TLV parse\n");
		return oml_fom_ack_nack(msg, NM_NACK_INCORR_STRUCT);
	}

	if (!TLVP_PRESENT(&tp, NM_ATT_ADM_STATE)) {
		LOGP(DOML, LOGL_ERROR, "Rx CHG ADM STATE: no ADM state attribute\n");
		return oml_fom_ack_nack(msg, NM_NACK_INCORR_STRUCT);
	}

	adm_state = *TLVP_VAL(&tp, NM_ATT_ADM_STATE);

	/* Step 1: Resolve MO by obj_class/obj_inst */
	mo = gsm_objclass2mo(bts, foh->obj_class, &foh->obj_inst);
	obj = gsm_objclass2obj(bts, foh->obj_class, &foh->obj_inst);
	if (!mo || !obj)
		return oml_fom_ack_nack(msg, NM_NACK_OBJINST_UNKN);

	/* Step 2: Do some global dependency/consistency checking */
	if (mo->nm_state.administrative == adm_state)
		LOGP(DOML, LOGL_NOTICE,
		     "ADM state already was %s\n",
		     get_value_string(abis_nm_adm_state_names, adm_state));

	/* Step 3: Ask BTS driver to apply the state chg */
	return bts_model_chg_adm_state(bts, mo, obj, adm_state);
}

/* GSM 12.21 section 8.10.1 */
static int  oml_rx_nm_meas_res_req(struct gsm_bts *bts, struct msgb *msg)
{
	struct abis_om_fom_hdr *foh = msgb_l3(msg);
	struct gsm_abis_mo *mo = &bts->gprs.cell.mo;
	struct tlv_parsed tp;
	int rc;
	uint8_t meas_id;

	LOGP(DOML, LOGL_DEBUG, "%s Rx MEAS RES REQ\n", gsm_abis_mo_name(mo));

	rc = oml_tlv_parse(&tp, foh->data, msgb_l3len(msg) - sizeof(*foh));
	if (rc < 0) {
		LOGP(DOML, LOGL_NOTICE, "New value for Attribute not supported\n");
		return oml_fom_ack_nack(msg, NM_NACK_INCORR_STRUCT);
	}

	if (!TLVP_PRESENT(&tp, NM_ATT_MEAS_TYPE)) {
		LOGP(DOML, LOGL_NOTICE, "%s NM_ATT_MEAS_TYPE not found\n", gsm_abis_mo_name(mo));
		return oml_fom_ack_nack(msg, NM_NACK_SPEC_IMPL_NOTSUPP);
	}

	/* Get measurement ID */
	meas_id = *TLVP_VAL(&tp, NM_ATT_MEAS_TYPE);

	/* Set start measurement ID flags*/
	switch(meas_id) {
	case NM_IPACC_MEAS_TYPE_CCCH:
		pcu_start_meas_flags |= PCU_IF_FLAG_START_MEAS_CCCH;
		break;
	case NM_IPACC_MEAS_TYPE_UL_TBF:
		pcu_start_meas_flags |= PCU_IF_FLAG_START_MEAS_UL_TBF;
		break;
	case NM_IPACC_MEAS_TYPE_DL_TBF:
		pcu_start_meas_flags |= PCU_IF_FLAG_START_MEAS_DL_TBF;
		break;
	case NM_IPACC_MEAS_TYPE_TBF_USE:
		pcu_start_meas_flags |= PCU_IF_FLAG_START_MEAS_TBF_USE;
		break;
	case NM_IPACC_MEAS_TYPE_LLC_USE:
		pcu_start_meas_flags |= PCU_IF_FLAG_START_MEAS_LLC_USE;
		break;
	case NM_IPACC_MEAS_TYPE_CS_CHG:
		pcu_start_meas_flags |= PCU_IF_FLAG_START_MEAS_CS_CHG;
		break;
	default:
		LOGP(DOML, LOGL_NOTICE, "%s Unsupported NM START MEASurement type received 0x%02x\n", gsm_abis_mo_name(mo), meas_id);

		/*send alarm to indicate PCU link is not ready */
		alarm_sig_data.mo = mo;
		alarm_sig_data.spare[0] = meas_id;
		osmo_signal_dispatch(SS_NM, S_NM_OML_BTS_UNKN_MEAS_REQ_ALARM, &alarm_sig_data);

		/*send START MEAS NACK */
		return oml_tx_nm_start_meas_ack_nack(mo, meas_id, NM_NACK_NOTH_REPORT_EXIST);
	}

	/* send request to PCU */
	rc = pcu_tx_nm_meas_res_req(bts, meas_id);
	if (rc < 0) {
		LOGP(DOML, LOGL_NOTICE, "%s PCU socket may not be ready for measurement ID (%d)\n", gsm_abis_mo_name(mo), meas_id);

		/*send alarm to indicate PCU link is not ready */
		alarm_sig_data.mo = mo;
		alarm_sig_data.spare[0] = meas_id;
		osmo_signal_dispatch(SS_NM, S_NM_OML_PCU_CONN_NOT_AVAIL_ALARM, &alarm_sig_data);

		/*send MEAS RES REQ NACK */
		return oml_tx_nm_meas_res_req_nack(mo, meas_id, NM_NACK_NOTH_REPORT_EXIST);
	}

	return 0;
}

/* GSM 12.21 section 8.10.3 */
static int  oml_rx_nm_start_meas(struct gsm_bts *bts, struct msgb *msg)
{
	struct abis_om_fom_hdr *foh = msgb_l3(msg);
	struct gsm_abis_mo *mo = &bts->gprs.cell.mo;
	struct tlv_parsed tp;
	int rc;
	uint8_t meas_id;

	LOGP(DOML, LOGL_DEBUG, "%s Rx START MEAS\n", gsm_abis_mo_name(mo));

	rc = oml_tlv_parse(&tp, foh->data, msgb_l3len(msg) - sizeof(*foh));
	if (rc < 0) {
		LOGP(DOML, LOGL_NOTICE, "New value for Attribute not supported\n");
		return oml_fom_ack_nack(msg, NM_NACK_INCORR_STRUCT);
	}

	if (!TLVP_PRESENT(&tp, NM_ATT_MEAS_TYPE)) {
		LOGP(DOML, LOGL_NOTICE, "New value for Attribute not supported\n");
		return oml_fom_ack_nack(msg, NM_NACK_SPEC_IMPL_NOTSUPP);
	}

	/* Get measurement ID */
	meas_id = *TLVP_VAL(&tp, NM_ATT_MEAS_TYPE);

	/* Set start measurement ID flags*/
	switch(meas_id) {
	case NM_IPACC_MEAS_TYPE_CCCH:
		pcu_start_meas_flags |= PCU_IF_FLAG_START_MEAS_CCCH;
		break;
	case NM_IPACC_MEAS_TYPE_UL_TBF:
		pcu_start_meas_flags |= PCU_IF_FLAG_START_MEAS_UL_TBF;
		break;
	case NM_IPACC_MEAS_TYPE_DL_TBF:
		pcu_start_meas_flags |= PCU_IF_FLAG_START_MEAS_DL_TBF;
		break;
	case NM_IPACC_MEAS_TYPE_TBF_USE:
		pcu_start_meas_flags |= PCU_IF_FLAG_START_MEAS_TBF_USE;
		break;
	case NM_IPACC_MEAS_TYPE_LLC_USE:
		pcu_start_meas_flags |= PCU_IF_FLAG_START_MEAS_LLC_USE;
		break;
	case NM_IPACC_MEAS_TYPE_CS_CHG:
		pcu_start_meas_flags |= PCU_IF_FLAG_START_MEAS_CS_CHG;
		break;
	default:
		LOGP(DOML, LOGL_NOTICE, "%s Unsupported NM START MEASurement type received 0x%02x\n", gsm_abis_mo_name(mo), meas_id);

		/*send alarm to indicate PCU link is not ready */
		alarm_sig_data.mo = mo;
		alarm_sig_data.spare[0] = meas_id;
		osmo_signal_dispatch(SS_NM, S_NM_OML_BTS_UNKN_START_MEAS_REQ_ALARM, &alarm_sig_data);

		/*send START MEAS NACK */
		return oml_tx_nm_start_meas_ack_nack(mo, meas_id, NM_NACK_NOTH_REPORT_EXIST);
	}

	/* send request to PCU */
	rc = pcu_tx_nm_start_meas(bts, meas_id, 0);
	if (rc < 0) {
		LOGP(DOML, LOGL_NOTICE, "%s PCU socket may not be ready\n", gsm_abis_mo_name(mo));
		/*send alarm to indicate PCU link is not ready */
		alarm_sig_data.mo = mo;
		alarm_sig_data.spare[0] = meas_id;
		osmo_signal_dispatch(SS_NM, S_NM_OML_PCU_CONN_NOT_AVAIL_ALARM, &alarm_sig_data);

		/*send START MEAS NACK */
		return oml_tx_nm_start_meas_ack_nack(mo, meas_id, NM_NACK_NOTH_REPORT_EXIST);
	}

	return 0;
}

static int  oml_rx_nm_stop_meas(struct gsm_bts *bts, struct msgb *msg)
{
	struct abis_om_fom_hdr *foh = msgb_l3(msg);
	struct gsm_abis_mo *mo = &bts->gprs.cell.mo;
	struct tlv_parsed tp;
	int rc;
	uint8_t meas_id;

	LOGP(DOML, LOGL_DEBUG, "%s Rx STOP MEAS\n", gsm_abis_mo_name(mo));

	rc = oml_tlv_parse(&tp, foh->data, msgb_l3len(msg) - sizeof(*foh));
	if (rc < 0) {
		LOGP(DOML, LOGL_NOTICE, "New value for Attribute not supported\n");
		return oml_fom_ack_nack(msg, NM_NACK_INCORR_STRUCT);
	}

	if (!TLVP_PRESENT(&tp, NM_ATT_MEAS_TYPE)) {
		LOGP(DOML, LOGL_NOTICE, "New value for Attribute not supported\n");
		return oml_fom_ack_nack(msg, NM_NACK_SPEC_IMPL_NOTSUPP);
	}

	/* Get measurement ID */
	meas_id = *TLVP_VAL(&tp, NM_ATT_MEAS_TYPE);

	/* Validate measurement ID */
	switch(meas_id) {
	case NM_IPACC_MEAS_TYPE_CCCH:
		pcu_start_meas_flags &= ~PCU_IF_FLAG_START_MEAS_CCCH;
		break;
	case NM_IPACC_MEAS_TYPE_UL_TBF:
		pcu_start_meas_flags &= ~PCU_IF_FLAG_START_MEAS_UL_TBF;
		break;
	case NM_IPACC_MEAS_TYPE_DL_TBF:
		pcu_start_meas_flags &= ~PCU_IF_FLAG_START_MEAS_DL_TBF;
		break;
	case NM_IPACC_MEAS_TYPE_TBF_USE:
		pcu_start_meas_flags &= ~PCU_IF_FLAG_START_MEAS_TBF_USE;
		break;
	case NM_IPACC_MEAS_TYPE_LLC_USE:
		pcu_start_meas_flags &= ~PCU_IF_FLAG_START_MEAS_LLC_USE;
		break;
	case NM_IPACC_MEAS_TYPE_CS_CHG:
		pcu_start_meas_flags &= ~PCU_IF_FLAG_START_MEAS_CS_CHG;
		break;
	default:
		LOGP(DOML, LOGL_NOTICE, "%s Unsupported NM STOP MEASurement type received 0x%02x\n", gsm_abis_mo_name(mo), meas_id);

		/*send alarm to indicate PCU link is not ready */
		alarm_sig_data.mo = mo;
		alarm_sig_data.spare[0] = meas_id;
		osmo_signal_dispatch(SS_NM, S_NM_OML_BTS_UNKN_STOP_MEAS_REQ_ALARM, &alarm_sig_data);

		/*send START MEAS NACK */
		return oml_tx_nm_start_meas_ack_nack(mo, meas_id, NM_NACK_NOTH_REPORT_EXIST);
	}

	/* send request to PCU */
	rc = pcu_tx_nm_stop_meas(bts, meas_id);
	if (rc < 0) {
		LOGP(DOML, LOGL_NOTICE, "%s PCU socket may not be ready\n", gsm_abis_mo_name(mo));

		/*send alarm to indicate PCU link is not ready */
		alarm_sig_data.mo = mo;
		alarm_sig_data.spare[0] = meas_id;
		osmo_signal_dispatch(SS_NM, S_NM_OML_PCU_CONN_NOT_AVAIL_ALARM, &alarm_sig_data);

		/*send STOP MEAS NACK */
		return oml_tx_nm_stop_meas_ack_nack(mo, meas_id, NM_NACK_NOTH_REPORT_EXIST);
	}

	return 0;
}

static int down_fom(struct gsm_bts *bts, struct msgb *msg)
{
	struct abis_om_fom_hdr *foh = msgb_l3(msg);
	struct gsm_bts_trx *trx;
	int ret;

	if (msgb_l2len(msg) < sizeof(*foh)) {
		LOGP(DOML, LOGL_NOTICE, "Formatted O&M message too short\n");
		return -EIO;
	}

	if (foh->obj_inst.bts_nr != 0 && foh->obj_inst.bts_nr != 0xff) {
		LOGP(DOML, LOGL_INFO, "Formatted O&M with BTS %d out of range.\n", foh->obj_inst.bts_nr);
		return oml_fom_ack_nack(msg, NM_NACK_BTSNR_UNKN);
	}

	switch (foh->msg_type) {
	case NM_MT_SET_BTS_ATTR:
		ret = oml_rx_set_bts_attr(bts, msg);
		break;
	case NM_MT_SET_RADIO_ATTR:
		trx = gsm_bts_trx_num(bts, foh->obj_inst.trx_nr);
		if (!trx)
			return oml_fom_ack_nack(msg, NM_NACK_TRXNR_UNKN);
		ret = oml_rx_set_radio_attr(trx, msg);
		break;
	case NM_MT_SET_CHAN_ATTR:
		trx = gsm_bts_trx_num(bts, foh->obj_inst.trx_nr);
		if (!trx)
			return oml_fom_ack_nack(msg, NM_NACK_TRXNR_UNKN);
		if (foh->obj_inst.ts_nr >= ARRAY_SIZE(trx->ts))
			return oml_fom_ack_nack(msg, NM_NACK_OBJINST_UNKN);
		ret = oml_rx_set_chan_attr(&trx->ts[foh->obj_inst.ts_nr], msg);
		break;
	case NM_MT_OPSTART:
		ret = oml_rx_opstart(bts, msg);
		break;
	case NM_MT_CHG_ADM_STATE:
		ret = oml_rx_chg_adm_state(bts, msg);
		break;
	case NM_MT_IPACC_SET_ATTR:
		ret = oml_ipa_set_attr(bts, msg);
		break;
	case NM_MT_START_MEAS:
		ret = oml_rx_nm_start_meas(bts, msg);
		break;
	case NM_MT_MEAS_RES_REQ:
		ret = oml_rx_nm_meas_res_req(bts, msg);
		break;
	case NM_MT_STOP_MEAS:
		ret = oml_rx_nm_stop_meas(bts, msg);
		break;
	default:
		LOGP(DOML, LOGL_INFO, "unknown Formatted O&M msg_type 0x%02x\n",
			foh->msg_type);
		ret = oml_fom_ack_nack(msg, NM_NACK_MSGTYPE_INVAL);
	}

	return ret;
}

/*
 * manufacturer related messages
 */

#ifndef TLVP_PRES_LEN /* old libosmocore */
#define TLVP_PRES_LEN(tp, tag, min_len) \
	(TLVP_PRESENT(tp, tag) && TLVP_LEN(tp, tag) >= min_len)
#endif

static int oml_ipa_mo_set_attr_nse(void *obj, struct tlv_parsed *tp)
{
	struct gsm_bts *bts = container_of(obj, struct gsm_bts, gprs.nse);

	if (TLVP_PRES_LEN(tp, NM_ATT_IPACC_NSEI, 2))
		bts->gprs.nse.nsei =
			ntohs(tlvp_val16_unal(tp, NM_ATT_IPACC_NSEI));

	if (TLVP_PRES_LEN(tp, NM_ATT_IPACC_NS_CFG, 7)) {
		memcpy(&bts->gprs.nse.timer,
		       TLVP_VAL(tp, NM_ATT_IPACC_NS_CFG), 7);
	}

	if (TLVP_PRES_LEN(tp, NM_ATT_IPACC_BSSGP_CFG, 11)) {
		memcpy(&bts->gprs.cell.timer,
		       TLVP_VAL(tp, NM_ATT_IPACC_BSSGP_CFG), 11);
	}

	osmo_signal_dispatch(SS_GLOBAL, S_NEW_NSE_ATTR, bts);

	return 0;
}

static int oml_ipa_mo_set_attr_cell(void *obj, struct tlv_parsed *tp)
{
	struct gsm_bts *bts = container_of(obj, struct gsm_bts, gprs.cell);
	struct gprs_rlc_cfg *rlcc = &bts->gprs.cell.rlc_cfg;
	const uint8_t *cur;
	uint16_t _cur_s;

	if (TLVP_PRES_LEN(tp, NM_ATT_IPACC_RAC, 1))
		bts->gprs.rac = *TLVP_VAL(tp, NM_ATT_IPACC_RAC);

	if (TLVP_PRES_LEN(tp, NM_ATT_IPACC_GPRS_PAGING_CFG, 2)) {
		cur = TLVP_VAL(tp, NM_ATT_IPACC_GPRS_PAGING_CFG);
		rlcc->paging.repeat_time = cur[0] * 50;
		rlcc->paging.repeat_count = cur[1];
	}

	if (TLVP_PRES_LEN(tp, NM_ATT_IPACC_BVCI, 2))
		bts->gprs.cell.bvci =
			htons(tlvp_val16_unal(tp, NM_ATT_IPACC_BVCI));

	if (TLVP_PRES_LEN(tp, NM_ATT_IPACC_RLC_CFG, 9)) {
		cur = TLVP_VAL(tp, NM_ATT_IPACC_RLC_CFG);
		rlcc->parameter[RLC_T3142] = cur[0];
		rlcc->parameter[RLC_T3169] = cur[1];
		rlcc->parameter[RLC_T3191] = cur[2];
		rlcc->parameter[RLC_T3193] = cur[3];
		rlcc->parameter[RLC_T3195] = cur[4];
		rlcc->parameter[RLC_N3101] = cur[5];
		rlcc->parameter[RLC_N3103] = cur[6];
		rlcc->parameter[RLC_N3105] = cur[7];
		rlcc->parameter[CV_COUNTDOWN] = cur[8];
	}

	if (TLVP_PRES_LEN(tp, NM_ATT_IPACC_CODING_SCHEMES, 2)) {
		int i;
		rlcc->cs_mask = 0;
		cur = TLVP_VAL(tp, NM_ATT_IPACC_CODING_SCHEMES);

		for (i = 0; i < 4; i++) {
			if (cur[0] & (1 << i))
				rlcc->cs_mask |= (1 << (GPRS_CS1+i));
		}
		if (cur[0] & 0x80)
			rlcc->cs_mask |= (1 << GPRS_MCS9);
		for (i = 0; i < 8; i++) {
			if (cur[1] & (1 << i))
				rlcc->cs_mask |= (1 << (GPRS_MCS1+i));
		}
	}

	if (TLVP_PRES_LEN(tp, NM_ATT_IPACC_RLC_CFG_2, 5)) {
		cur = TLVP_VAL(tp, NM_ATT_IPACC_RLC_CFG_2);
		memcpy(&_cur_s, cur, 2);
		rlcc->parameter[T_DL_TBF_EXT] = ntohs(_cur_s) * 10;
		cur += 2;
		memcpy(&_cur_s, cur, 2);
		rlcc->parameter[T_UL_TBF_EXT] = ntohs(_cur_s) * 10;
		cur += 2;
		rlcc->initial_cs = *cur;
	}

	if (TLVP_PRES_LEN(tp, NM_ATT_IPACC_RLC_CFG_3, 1)) {
		rlcc->initial_mcs = *TLVP_VAL(tp, NM_ATT_IPACC_RLC_CFG_3);
	}

	osmo_signal_dispatch(SS_GLOBAL, S_NEW_CELL_ATTR, bts);

	return 0;
}

static int oml_ipa_mo_set_attr_nsvc(struct gsm_bts_gprs_nsvc *nsvc,
				    struct tlv_parsed *tp)
{
	if (TLVP_PRES_LEN(tp, NM_ATT_IPACC_NSVCI, 2))
		nsvc->nsvci = ntohs(tlvp_val16_unal(tp, NM_ATT_IPACC_NSVCI));

	if (TLVP_PRES_LEN(tp, NM_ATT_IPACC_NS_LINK_CFG, 8)) {
		const uint8_t *cur = TLVP_VAL(tp, NM_ATT_IPACC_NS_LINK_CFG);
		uint16_t _cur_s;
		uint32_t _cur_l;

		memcpy(&_cur_s, cur, 2);
		nsvc->remote_port = ntohs(_cur_s);
		cur += 2;
		memcpy(&_cur_l, cur, 4);
		nsvc->remote_ip = ntohl(_cur_l);
		cur += 4;
		memcpy(&_cur_s, cur, 2);
		nsvc->local_port = ntohs(_cur_s);
	}

	osmo_signal_dispatch(SS_GLOBAL, S_NEW_NSVC_ATTR, nsvc);

	return 0;
}

static int oml_ipa_mo_set_attr(struct gsm_bts *bts, struct gsm_abis_mo *mo,
				void *obj, struct tlv_parsed *tp)
{
	int rc;

	switch (mo->obj_class) {
	case NM_OC_GPRS_NSE:
		rc = oml_ipa_mo_set_attr_nse(obj, tp);
		break;
	case NM_OC_GPRS_CELL:
		rc = oml_ipa_mo_set_attr_cell(obj, tp);
		break;
	case NM_OC_GPRS_NSVC:
		rc = oml_ipa_mo_set_attr_nsvc(obj, tp);
		break;
	default:
		rc = NM_NACK_OBJINST_UNKN;
	}

	return rc;
}

static int oml_ipa_set_attr(struct gsm_bts *bts, struct msgb *msg)
{
	struct abis_om_fom_hdr *foh = msgb_l3(msg);
	struct gsm_abis_mo *mo;
	struct tlv_parsed tp;
	void *obj;
	int rc;

	abis_nm_debugp_foh(DOML, foh);
	DEBUGPC(DOML, "Rx IPA SET ATTR\n");

	rc = oml_tlv_parse(&tp, foh->data, msgb_l3len(msg) - sizeof(*foh));
	if (rc < 0)
		return oml_fom_ack_nack(msg, NM_NACK_INCORR_STRUCT);

	/* Resolve MO by obj_class/obj_inst */
	mo = gsm_objclass2mo(bts, foh->obj_class, &foh->obj_inst);
	obj = gsm_objclass2obj(bts, foh->obj_class, &foh->obj_inst);
	if (!mo || !obj)
		return oml_fom_ack_nack(msg, NM_NACK_OBJINST_UNKN);

	rc = oml_ipa_mo_set_attr(bts, mo, obj, &tp);

	return oml_fom_ack_nack(msg, rc);
}

static int rx_oml_ipa_rsl_connect(struct gsm_bts_trx *trx, struct msgb *msg,
				  struct tlv_parsed *tp)
{
	struct e1inp_sign_link *oml_link = trx->bts->oml_link;
	uint16_t port = IPA_TCP_PORT_RSL;
	uint32_t ip = get_signlink_remote_ip(oml_link);
	struct in_addr in;
	int rc;

	uint8_t stream_id = 0;

	if (TLVP_PRESENT(tp, NM_ATT_IPACC_DST_IP)) {
		ip = ntohl(tlvp_val32_unal(tp, NM_ATT_IPACC_DST_IP));
	}
	if (TLVP_PRESENT(tp, NM_ATT_IPACC_DST_IP_PORT)) {
		port = ntohs(tlvp_val16_unal(tp, NM_ATT_IPACC_DST_IP_PORT));
	}
	if (TLVP_PRESENT(tp, NM_ATT_IPACC_STREAM_ID)) {
		stream_id = *TLVP_VAL(tp, NM_ATT_IPACC_STREAM_ID);
	}

	in.s_addr = htonl(ip);
	LOGP(DOML, LOGL_INFO, "Rx IPA RSL CONNECT IP=%s PORT=%u STREAM=0x%02x\n", 
		inet_ntoa(in), port, stream_id);

	rc = e1inp_ipa_bts_rsl_connect_n(oml_link->ts->line, inet_ntoa(in), port,
		trx->nr);
	if (rc < 0) {
		LOGP(DOML, LOGL_ERROR, "Error in abis_open(RSL): %d\n", rc);
		return oml_fom_ack_nack(msg, NM_NACK_CANT_PERFORM);
	}

	return oml_fom_ack_nack(msg, 0);
}

static int down_mom(struct gsm_bts *bts, struct msgb *msg)
{
	struct abis_om_hdr *oh = msgb_l2(msg);
	struct abis_om_fom_hdr *foh;
	struct gsm_bts_trx *trx;
	uint8_t idstrlen = oh->data[0];
	struct tlv_parsed tp;
	int ret;

	if (msgb_l2len(msg) < sizeof(*foh)) {
		LOGP(DOML, LOGL_NOTICE, "Manufacturer O&M message too short\n");
		return -EIO;
	}

	if (strncmp((char *)&oh->data[1], abis_nm_ipa_magic, idstrlen)) {
		LOGP(DOML, LOGL_ERROR, "Manufacturer OML message != ipaccess not supported\n");
		return -EINVAL;
	}

	msg->l3h = oh->data + 1 + idstrlen;
	foh = (struct abis_om_fom_hdr *) msg->l3h;

	if (foh->obj_inst.bts_nr != 0 && foh->obj_inst.bts_nr != 0xff) {
		LOGP(DOML, LOGL_INFO, "Manufacturer O&M with BTS %d out of range.\n", foh->obj_inst.bts_nr);
		return oml_fom_ack_nack(msg, NM_NACK_BTSNR_UNKN);
	}

	ret = oml_tlv_parse(&tp, foh->data, oh->length - sizeof(*foh));
	if (ret < 0) {
		LOGP(DOML, LOGL_ERROR, "TLV parse error %d\n", ret);
		return oml_fom_ack_nack(msg, NM_NACK_BTSNR_UNKN);
	}

	abis_nm_debugp_foh(DOML, foh);
	DEBUGPC(DOML, "Rx IPACCESS(0x%02x): ", foh->msg_type);

	switch (foh->msg_type) {
	case NM_MT_IPACC_RSL_CONNECT:
		trx = gsm_bts_trx_num(bts, foh->obj_inst.trx_nr);
		ret = rx_oml_ipa_rsl_connect(trx, msg, &tp);
		break;
	case NM_MT_IPACC_SET_ATTR:
		ret = oml_ipa_set_attr(bts, msg);
		break;
	default:
		LOGP(DOML, LOGL_INFO, "Manufacturer Formatted O&M msg_type 0x%02x\n",
			foh->msg_type);
		ret = oml_fom_ack_nack(msg, NM_NACK_MSGTYPE_INVAL);
	}

	return ret;
}

/* incoming OML message from BSC */
int down_oml(struct gsm_bts *bts, struct msgb *msg)
{
	struct abis_om_hdr *oh = msgb_l2(msg);
	int ret = 0;

	if (msgb_l2len(msg) < 1) {
		LOGP(DOML, LOGL_NOTICE, "OML message too short\n");
		msgb_free(msg);
		return -EIO;
	}
	msg->l3h = (unsigned char *)oh + sizeof(*oh);

	switch (oh->mdisc) {
	case ABIS_OM_MDISC_FOM:
		if (msgb_l2len(msg) < sizeof(*oh)) {
			LOGP(DOML, LOGL_NOTICE, "Formatted O&M message too short\n");
			ret = -EIO;
			break;
		}
		ret = down_fom(bts, msg);
		break;
	case ABIS_OM_MDISC_MANUF:
		if (msgb_l2len(msg) < sizeof(*oh)) {
			LOGP(DOML, LOGL_NOTICE, "Manufacturer O&M message too short\n");
			ret = -EIO;
			break;
		}
		ret = down_mom(bts, msg);
		break;
	default:
		LOGP(DOML, LOGL_NOTICE, "unknown OML msg_discr 0x%02x\n",
			oh->mdisc);
		ret = -EINVAL;
	}

	msgb_free(msg);

	return ret;
}

int oml_init(void)
{
	DEBUGP(DOML, "Initializing OML attribute definitions\n");
	tlv_def_patch(&abis_nm_att_tlvdef_ipa, &abis_nm_att_tlvdef);

	return 0;
}

static int handle_oml_fail_evt_rep_sig(unsigned int subsys, unsigned int signal,
			void *handler_data, void *_signal_data)
{
	struct oml_fail_evt_rep_sig_data *sig_data = _signal_data;
	int rc = 0;
	unsigned int res;
	char log_msg[100];

	if (subsys != SS_NM)
		return 0;

	switch (signal) {
	case S_NM_OML_PCU_CONN_LOST_ALARM:
		snprintf(log_msg, 100, "PCU socket has LOST connection\n");
		sig_data->add_text = (char*)&log_msg;

		rc = oml_tx_nm_fail_evt_rep(sig_data->mo,
				NM_EVT_COMM_FAIL,
				NM_SEVER_CRITICAL,
				NM_PCAUSE_T_MANUF,
				NM_EVT_CAUSE_CRIT_SW_FATAL,
				sig_data->add_text);
		break;

	case S_NM_OML_PCU_CONN_LOST_CEASED:
		snprintf(log_msg, 100, "PCU socket has LOST connection\n");
		sig_data->add_text = (char*)&log_msg;

		rc = oml_tx_nm_fail_evt_rep(sig_data->mo,
				NM_EVT_COMM_FAIL,
				NM_SEVER_CEASED,
				NM_PCAUSE_T_MANUF,
				NM_EVT_CAUSE_CRIT_SW_FATAL,
				sig_data->add_text);
		break;

	case S_NM_OML_BTS_RX_UNKN_PCU_MSG_ALARM:
		snprintf(log_msg, 100, "Received unknown PCU msg type 0x%02x\n", sig_data->spare[0]);
		sig_data->add_text = (char*)&log_msg;

		rc = oml_tx_nm_fail_evt_rep(sig_data->mo,
				NM_EVT_COMM_FAIL,
				NM_SEVER_MAJOR,
				NM_PCAUSE_T_MANUF,
				NM_EVT_CAUSE_MAJ_UKWN_PCU_MSG,
				sig_data->add_text);
		break;

	case S_NM_OML_BTS_RSL_FAILED_ALARM:
		snprintf(log_msg, 100, "Failed to establish RSL link (%d)\n", sig_data->mo->obj_inst.trx_nr);
		sig_data->add_text = (char*)&log_msg;

		rc = oml_tx_nm_fail_evt_rep(sig_data->mo,
				NM_EVT_PROC_FAIL,
				NM_SEVER_CRITICAL,
				NM_PCAUSE_T_MANUF,
				NM_EVT_CAUSE_MAJ_RSL_FAIL,
				sig_data->add_text);
		break;

	case S_NM_OML_BTS_RF_DEACT_FAILED_ALARM:
		snprintf(log_msg, 100, "Failed to deactivate RF (%d)\n", sig_data->mo->obj_inst.trx_nr);
		sig_data->add_text = (char*)&log_msg;

		rc = oml_tx_nm_fail_evt_rep(sig_data->mo,
				NM_EVT_PROC_FAIL,
				NM_SEVER_CRITICAL,
				NM_PCAUSE_T_MANUF,
				NM_EVT_CAUSE_MAJ_DEACT_RF_FAIL,
				sig_data->add_text);
		break;

	case S_NM_OML_BTS_RX_UNKN_L1SAP_UP_MSG_ALARM:
		memcpy(&res, sig_data->spare, sizeof(unsigned int));
		snprintf(log_msg, 100, "unknown prim %d L1SAP UP trx %d\n", sig_data->mo->obj_inst.trx_nr, res);
		sig_data->add_text = (char*)&log_msg;

		rc = oml_tx_nm_fail_evt_rep(sig_data->mo,
				NM_EVT_COMM_FAIL,
				NM_SEVER_MAJOR,
				NM_PCAUSE_T_MANUF,
				NM_EVT_CAUSE_MAJ_UKWN_UL_MSG,
				sig_data->add_text);
		break;

	case S_NM_OML_BTS_RX_UNKN_L1SAP_DOWN_MSG_ALARM:
		memcpy(&res, sig_data->spare, sizeof(unsigned int));
		snprintf(log_msg, 100, "unknown prim %d L1SAP DOWN trx %d\n", sig_data->mo->obj_inst.trx_nr, res);
		sig_data->add_text = (char*)&log_msg;

		rc = oml_tx_nm_fail_evt_rep(sig_data->mo,
				NM_EVT_COMM_FAIL,
				NM_SEVER_MAJOR,
				NM_PCAUSE_T_MANUF,
				NM_EVT_CAUSE_MAJ_UKWN_DL_MSG,
				sig_data->add_text);
		break;

	case S_NM_OML_BTS_RX_SIGINT_MSG_ALARM:
		rc = oml_tx_nm_fail_evt_rep(sig_data->mo,
				NM_EVT_PROC_FAIL,
				NM_SEVER_CRITICAL,
				NM_PCAUSE_T_MANUF,
				NM_EVT_CAUSE_CRIT_PROC_STOP,
				"BTS: signal SIGINT received -> shutdown\n");
		break;

	case S_NM_OML_BTS_RX_SIGX_MSG_ALARM:
		snprintf(log_msg, 100, "BTS: signal %d received\n", sig_data->spare[0]);
		sig_data->add_text = (char*)&log_msg;

		rc = oml_tx_nm_fail_evt_rep(sig_data->mo,
				NM_EVT_PROC_FAIL,
				NM_SEVER_CRITICAL,
				NM_PCAUSE_T_MANUF,
				NM_EVT_CAUSE_CRIT_PROC_STOP,
				sig_data->add_text);
		break;

	case S_NM_OML_BTS_DSP_ALIVE_ALARM:
		memcpy(&res, sig_data->spare, sizeof(unsigned int));
		snprintf(log_msg, 100, "Timeout waiting for SYS primitive %s (%d)\n",
				get_value_string(lc15bts_sysprim_names, res + 1), sig_data->mo->obj_inst.trx_nr);
		sig_data->add_text = (char*)&log_msg;

		rc = oml_tx_nm_fail_evt_rep(sig_data->mo,
				NM_EVT_PROC_FAIL,
				NM_SEVER_CRITICAL,
				NM_PCAUSE_T_MANUF,
				NM_EVT_CAUSE_CRIT_DSP_FATAL,
				sig_data->add_text);
		break;

	case S_NM_OML_BTS_UNKN_MPH_INFO_REQ_ALARM:
		memcpy(&res, sig_data->spare, sizeof(unsigned int));
		snprintf(log_msg, 100, "unknown MPH-INFO.req %d\n", res);
		sig_data->add_text = (char*)&log_msg;

		rc = oml_tx_nm_fail_evt_rep(sig_data->mo,
				NM_EVT_COMM_FAIL,
				NM_SEVER_MAJOR,
				NM_PCAUSE_T_MANUF,
				NM_EVT_CAUSE_MAJ_UKWN_MPH_MSG,
				sig_data->add_text);
		break;

	case S_NM_OML_BTS_PAG_TBL_FULL_ALARM:
		rc = oml_tx_nm_fail_evt_rep(sig_data->mo,
				NM_EVT_ENV_FAIL,
				NM_SEVER_MINOR,
				NM_PCAUSE_T_MANUF,
				NM_EVT_CAUSE_MIN_PAG_TAB_FULL,
				"BTS page table is full\n");
		break;

	case S_NM_OML_BTS_FAIL_RTP_SOCK_ALARM:
		snprintf(log_msg, 100, "IPAC Failed to create RTP/RTCP sockets, trx=%d, ts=%d, ss=%d\n",
				sig_data->mo->obj_inst.trx_nr,
				sig_data->spare[0],
				sig_data->spare[1]);

		sig_data->add_text = (char*)&log_msg;

		rc = oml_tx_nm_fail_evt_rep(sig_data->mo,
				NM_EVT_COMM_FAIL,
				NM_SEVER_CRITICAL,
				NM_PCAUSE_T_MANUF,
				NM_EVT_CAUSE_CRIT_RTP_CREATE_FAIL,
				sig_data->add_text);
		break;

	case S_NM_OML_BTS_FAIL_RTP_BIND_ALARM:
		snprintf(log_msg, 100, "IPAC Failed to bind RTP/RTCP sockets, trx=%d, ts=%d, ss=%d\n",
				sig_data->mo->obj_inst.trx_nr,
				sig_data->spare[0],
				sig_data->spare[1]);

		sig_data->add_text = (char*)&log_msg;

		rc = oml_tx_nm_fail_evt_rep(sig_data->mo,
				NM_EVT_COMM_FAIL,
				NM_SEVER_CRITICAL,
				NM_PCAUSE_T_MANUF,
				NM_EVT_CAUSE_CRIT_RTP_BIND_FAIL,
				sig_data->add_text);
		break;

	case S_NM_OML_BTS_NO_RTP_SOCK_ALARM:
		snprintf(log_msg, 100, "Rx RSL IPAC MDCX, but we have no RTP socket! trx=%d, ts=%d, ss=%d\n",
				sig_data->mo->obj_inst.trx_nr,
				sig_data->spare[0],
				sig_data->spare[1]);

		sig_data->add_text = (char*)&log_msg;

		rc = oml_tx_nm_fail_evt_rep(sig_data->mo,
				NM_EVT_COMM_FAIL,
				NM_SEVER_CRITICAL,
				NM_PCAUSE_T_MANUF,
				NM_EVT_CAUSE_CRIT_RTP_NO_SOCK,
				sig_data->add_text);
		break;

	case S_NM_OML_BTS_NO_CALIB_PATH_ALARM:
		snprintf(log_msg, 100, "Calibration file path for trx=%d not specified\n", sig_data->mo->obj_inst.trx_nr);
		sig_data->add_text = (char*)&log_msg;

		rc = oml_tx_nm_fail_evt_rep(sig_data->mo,
				NM_EVT_PROC_FAIL,
				NM_SEVER_CRITICAL,
				NM_PCAUSE_T_MANUF,
				NM_EVT_CAUSE_CRIT_BAD_CALIB_PATH,
				sig_data->add_text);
		break;


	case S_NM_OML_BTS_FAIL_OPEN_CALIB_ALARM:
		memcpy(&res, sig_data->spare, sizeof(int));
		snprintf(log_msg, 100, "Failed to open L1 calibration table %s -> failed (%d)\n",
				sig_data->add_text,
				res);
		sig_data->add_text = (char*)&log_msg;

		rc = oml_tx_nm_fail_evt_rep(sig_data->mo,
				NM_EVT_PROC_FAIL,
				NM_SEVER_CRITICAL,
				NM_PCAUSE_T_MANUF,
				NM_EVT_CAUSE_CRIT_OPEN_CALIB_FAIL,
				sig_data->add_text);
		break;

	case S_NM_OML_BTS_FAIL_VERIFY_CALIB_ALARM:
		memcpy(&res, sig_data->spare, sizeof(int));
		snprintf(log_msg, 100, "Verify L1 calibration table %s -> failed (%d)\n",
				sig_data->add_text,
				res);
		sig_data->add_text = (char*)&log_msg;

		rc = oml_tx_nm_fail_evt_rep(sig_data->mo,
				NM_EVT_PROC_FAIL,
				NM_SEVER_CRITICAL,
				NM_PCAUSE_T_MANUF,
				NM_EVT_CAUSE_CRIT_VERIFY_CALIB_FAIL,
				sig_data->add_text);
		break;

	case S_NM_OML_BTS_RX_PCU_FAIL_EVT_ALARM:
		rc = oml_tx_nm_fail_evt_rep(sig_data->mo,
				sig_data->event_type,
				sig_data->event_serverity,
				sig_data->cause_type,
				sig_data->event_cause,
				sig_data->add_text);
		break;

	case S_NM_OML_BTS_UNKN_START_MEAS_REQ_ALARM:
		snprintf(log_msg, 100, "Unsupported START MEASurement ID (0x%02x)\n", sig_data->spare[0]);
		sig_data->add_text = (char*)&log_msg;

		rc = oml_tx_nm_fail_evt_rep(sig_data->mo,
				NM_EVT_COMM_FAIL,
				NM_SEVER_MAJOR,
				NM_PCAUSE_T_MANUF,
				NM_EVT_CAUSE_MAJ_UKWN_MEAS_START_MSG,
				sig_data->add_text);
		break;

	case S_NM_OML_BTS_UNKN_STOP_MEAS_REQ_ALARM:
		snprintf(log_msg, 100, "Unsupported STOP MEASurement ID (0x%02x)\n", sig_data->spare[0]);
		sig_data->add_text = (char*)&log_msg;

		rc = oml_tx_nm_fail_evt_rep(sig_data->mo,
				NM_EVT_COMM_FAIL,
				NM_SEVER_MAJOR,
				NM_PCAUSE_T_MANUF,
				NM_EVT_CAUSE_MAJ_UKWN_MEAS_STOP_MSG,
				sig_data->add_text);
		break;

	case S_NM_OML_BTS_UNKN_MEAS_REQ_ALARM:
		snprintf(log_msg, 100, "Unsupported MEASurement REQuest ID (0x%02x)\n", sig_data->spare[0]);
		sig_data->add_text = (char*)&log_msg;

		rc = oml_tx_nm_fail_evt_rep(sig_data->mo,
				NM_EVT_COMM_FAIL,
				NM_SEVER_MAJOR,
				NM_PCAUSE_T_MANUF,
				NM_EVT_CAUSE_MAJ_UKWN_MEAS_REQ_MSG,
				sig_data->add_text);
		break;

	default:
		break;
	}

	sig_data->rc = rc;
	return 0;
}

int oml_failure_report_init(void *handler)
{
	/* register for failure report events */
	osmo_signal_register_handler(SS_NM, handle_oml_fail_evt_rep_sig, handler);

	return 0;
}
