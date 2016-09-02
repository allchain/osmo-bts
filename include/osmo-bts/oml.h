#ifndef _OML_H
#define _OML_H
#include <osmo-bts/pcuif_proto.h>
#include "openbsc/signal.h"

struct gsm_bts;
struct gsm_abis_mo;
struct msgb;
struct gsm_lchan;

enum oml_fail_evt_rep_sig {
	S_NM_OML_PCU_CONN_LOST_ALARM		= 0x0a1a,
	S_NM_OML_PCU_CONN_LOST_CEASED,
	S_NM_OML_PCU_CONN_NOT_AVAIL_ALARM,
	S_NM_OML_BTS_RX_UNKN_PCU_MSG_ALARM,
	S_NM_OML_BTS_RSL_FAILED_ALARM,
	S_NM_OML_BTS_RF_DEACT_FAILED_ALARM,
	S_NM_OML_BTS_RX_SIGINT_MSG_ALARM,
	S_NM_OML_BTS_RX_SIGX_MSG_ALARM,
	S_NM_OML_BTS_DSP_ALIVE_ALARM,
	S_NM_OML_BTS_UNKN_MPH_INFO_REQ_ALARM,
	S_NM_OML_BTS_RX_UNKN_L1SAP_UP_MSG_ALARM,
	S_NM_OML_BTS_RX_UNKN_L1SAP_DOWN_MSG_ALARM,
	S_NM_OML_BTS_PAG_TBL_FULL_ALARM,
	S_NM_OML_BTS_FAIL_RTP_SOCK_ALARM,
	S_NM_OML_BTS_FAIL_RTP_BIND_ALARM,
	S_NM_OML_BTS_NO_RTP_SOCK_ALARM,
	S_NM_OML_BTS_FAIL_OPEN_CALIB_ALARM,
	S_NM_OML_BTS_FAIL_VERIFY_CALIB_ALARM,
	S_NM_OML_BTS_NO_CALIB_PATH_ALARM,
	S_NM_OML_BTS_RX_PCU_FAIL_EVT_ALARM,
	S_NM_OML_BTS_UNKN_START_MEAS_REQ_ALARM,
	S_NM_OML_BTS_UNKN_STOP_MEAS_REQ_ALARM,
	S_NM_OML_BTS_UNKN_MEAS_REQ_ALARM,
};

struct oml_fail_evt_rep_sig_data {
	struct gsm_abis_mo *mo;
	uint8_t event_type;
	uint8_t event_serverity;
	uint8_t cause_type;
	uint16_t event_cause;
	char *add_text;
	int rc;
	uint8_t spare[4];
};

struct oml_alarm_list {
	struct llist_head list; /* List of sent failure alarm report */
	uint16_t alarm_signal;	/* Failure alarm report signal cause */
};

enum abis_nm_msgtype_ipacc_appended {
	NM_MT_IPACC_START_MEAS_ACK 		= 0xde,
	NM_MT_IPACC_MEAS_RES_REQ_NACK		= 0xfc,
	NM_MT_IPACC_START_MEAS_NACK		= 0xfd,
	NM_MT_IPACC_STOP_MEAS_ACK 		= 0xdf,
	NM_MT_IPACC_STOP_MEAS_NACK		= 0xfe,
};

enum abis_nm_ipacc_meas_type {
	NM_IPACC_MEAS_TYPE_CCCH 		= 0x45,
	NM_IPACC_MEAS_TYPE_UL_TBF 		= 0x49,
	NM_IPACC_MEAS_TYPE_DL_TBF 		= 0x4a,
	NM_IPACC_MEAS_TYPE_TBF_USE 		= 0x4c,
	NM_IPACC_MEAS_TYPE_LLC_USE 		= 0x4d,
	NM_IPACC_MEAS_TYPE_CS_CHG 		= 0x50,
	NM_IPACC_MEAS_TYPE_UNKN
};

enum abis_nm_failure_event_causes {
	/* Critical causes */
	NM_EVT_CAUSE_CRIT_SW_FATAL 		= 0x3000,
	NM_EVT_CAUSE_CRIT_DSP_FATAL 		= 0x3001,
	NM_EVT_CAUSE_CRIT_PROC_STOP 		= 0x3006,
	NM_EVT_CAUSE_CRIT_RTP_CREATE_FAIL	= 0x332a,
	NM_EVT_CAUSE_CRIT_RTP_BIND_FAIL		= 0x332b,
	NM_EVT_CAUSE_CRIT_RTP_NO_SOCK		= 0x332c,
	NM_EVT_CAUSE_CRIT_BAD_CALIB_PATH	= 0x3401,
	NM_EVT_CAUSE_CRIT_OPEN_CALIB_FAIL 	= 0x3403,
	NM_EVT_CAUSE_CRIT_VERIFY_CALIB_FAIL 	= 0x3404,
	/* Major causes */
	NM_EVT_CAUSE_MAJ_UKWN_PCU_MSG		= 0x3002,
	NM_EVT_CAUSE_MAJ_UKWN_DL_MSG		= 0x3003,
	NM_EVT_CAUSE_MAJ_UKWN_UL_MSG		= 0x3004,
	NM_EVT_CAUSE_MAJ_UKWN_MPH_MSG		= 0x3005,
	NM_EVT_CAUSE_MAJ_UKWN_MEAS_START_MSG	= 0x3007,
	NM_EVT_CAUSE_MAJ_UKWN_MEAS_REQ_MSG	= 0x3008,
	NM_EVT_CAUSE_MAJ_UKWN_MEAS_STOP_MSG	= 0x3009,
	NM_EVT_CAUSE_MAJ_RSL_FAIL		= 0x3309,
	NM_EVT_CAUSE_MAJ_DEACT_RF_FAIL 		= 0x330a,
	/* Minor causes */
	NM_EVT_CAUSE_MIN_PAG_TAB_FULL		= 0x3402,
	/* Warning causes */
	NM_EVT_CAUSE_WARN_SW_WARN		= 0x0001,

};

extern struct oml_fail_evt_rep_sig_data alarm_sig_data;

int oml_init(void);
int down_oml(struct gsm_bts *bts, struct msgb *msg);

struct msgb *oml_msgb_alloc(void);
int oml_send_msg(struct msgb *msg, int is_mauf);
int oml_mo_send_msg(struct gsm_abis_mo *mo, struct msgb *msg, uint8_t msg_type);
int oml_mo_opstart_ack(struct gsm_abis_mo *mo);
int oml_mo_opstart_nack(struct gsm_abis_mo *mo, uint8_t nack_cause);
int oml_mo_statechg_ack(struct gsm_abis_mo *mo);
int oml_mo_statechg_nack(struct gsm_abis_mo *mo, uint8_t nack_cause);

/* Change the state and send STATE CHG REP */
int oml_mo_state_chg(struct gsm_abis_mo *mo, int op_state, int avail_state);

/* First initialization of MO, does _not_ generate state changes */
void oml_mo_state_init(struct gsm_abis_mo *mo, int op_state, int avail_state);

/* Update admin state and send ACK/NACK */
int oml_mo_rf_lock_chg(struct gsm_abis_mo *mo, uint8_t mute_state[8],
		       int success);

/* Transmit STATE CHG REP even if there was no state change */
int oml_tx_state_changed(struct gsm_abis_mo *mo);

int oml_mo_tx_sw_act_rep(struct gsm_abis_mo *mo);

int oml_fom_ack_nack(struct msgb *old_msg, uint8_t cause);

int oml_mo_fom_ack_nack(struct gsm_abis_mo *mo, uint8_t orig_msg_type,
			uint8_t cause);

/* Configure LAPDm T200 timers for this lchan according to OML */
int oml_set_lchan_t200(struct gsm_lchan *lchan);
extern const unsigned int oml_default_t200_ms[7];

/* Transmit failure event report */
int oml_tx_nm_fail_evt_rep(struct gsm_abis_mo *mo, uint8_t event_type, uint8_t event_serverity, uint8_t cause_type, uint16_t event_cause, char *add_text);

/* Initialize failure report signalling */
int oml_failure_report_init(void *handler);

/* NM measurement related messages */
int oml_tx_nm_start_meas_ack_nack(struct gsm_abis_mo *mo, uint8_t meas_id, uint8_t nack_cause);
int oml_tx_nm_meas_res_req_nack(struct gsm_abis_mo *mo, uint8_t meas_id, uint8_t nack_cause);
int oml_tx_nm_meas_res_resp(struct gsm_abis_mo *mo, struct gsm_pcu_if_meas_resp meas_resp);
int oml_tx_nm_stop_meas_ack_nack(struct gsm_abis_mo *mo, uint8_t meas_id, uint8_t nack_cause);

#endif // _OML_H */
