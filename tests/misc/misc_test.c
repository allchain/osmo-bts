/* testing misc code */

/* (C) 2011 by Holger Hans Peter Freyther
 * (C) 2014 by sysmocom s.f.m.c. GmbH
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <osmo-bts/bts.h>
#include <osmo-bts/msg_utils.h>
#include <osmo-bts/logging.h>

#include <osmocom/gsm/protocol/ipaccess.h>

#include <stdlib.h>
#include <stdio.h>

static const uint8_t ipa_rsl_connect[] = {
	0x00, 0x1c, 0xff, 0x10, 0x80, 0x00, 0x0a, 0x0d,
	0x63, 0x6f, 0x6d, 0x2e, 0x69, 0x70, 0x61, 0x63,
	0x63, 0x65, 0x73, 0x73, 0x00, 0xe0, 0x04, 0x00,
	0x00, 0xff, 0x85, 0x00, 0x81, 0x0b, 0xbb
};

static void test_msg_utils_ipa(void)
{
	struct msgb *msg;
	int rc, size;

	printf("Testing IPA structure\n");

	msg = msgb_alloc(sizeof(ipa_rsl_connect), "IPA test");
	msg->l1h = msgb_put(msg, sizeof(ipa_rsl_connect));
	memcpy(msg->l1h, ipa_rsl_connect, sizeof(ipa_rsl_connect));
	rc = msg_verify_ipa_structure(msg);
	OSMO_ASSERT(rc == 0);
	msgb_free(msg);

	/* test truncated messages and they should fail */
	for (size = sizeof(ipa_rsl_connect) - 1; size >= 0; --size) {
		msg = msgb_alloc(sizeof(ipa_rsl_connect) - 1, "IPA test");
		msg->l1h = msgb_put(msg, size);
		memcpy(msg->l1h, ipa_rsl_connect, size);
		rc = msg_verify_ipa_structure(msg);
		OSMO_ASSERT(rc == -1);
		msgb_free(msg);
	}

	/* change the type of the message */
	msg = msgb_alloc(sizeof(ipa_rsl_connect), "IPA test");
	msg->l1h = msgb_put(msg, sizeof(ipa_rsl_connect));
	memcpy(msg->l1h, ipa_rsl_connect, sizeof(ipa_rsl_connect));
	msg->l1h[2] = 0x23;
	rc = msg_verify_ipa_structure(msg);
	OSMO_ASSERT(rc == -1);
	msgb_free(msg);
}

static void test_msg_utils_oml(void)
{
	static const size_t hh_size = sizeof(struct ipaccess_head);
	struct msgb *msg;
	int rc, size;

	printf("Testing OML structure\n");

	msg = msgb_alloc(sizeof(ipa_rsl_connect) - hh_size, "IPA test");
	msg->l2h = msgb_put(msg, sizeof(ipa_rsl_connect) - hh_size);
	memcpy(msg->l2h, ipa_rsl_connect + hh_size, sizeof(ipa_rsl_connect) - hh_size);
	rc = msg_verify_oml_structure(msg);
	OSMO_ASSERT(rc == OML_MSG_TYPE_IPA);
	msgb_free(msg);

	/* test truncated messages and they should fail */
	for (size = sizeof(ipa_rsl_connect) - hh_size - 1; size >=0; --size) {
		msg = msgb_alloc(size, "IPA test");
		msg->l2h = msgb_put(msg, size);
		memcpy(msg->l2h, ipa_rsl_connect + hh_size, size);
		rc = msg_verify_oml_structure(msg);
		OSMO_ASSERT(rc == -1);
		msgb_free(msg);
	}
}

static void test_sacch_get(void)
{
	struct gsm_lchan lchan;
	int i, off;

	printf("Testing lchan_sacch_get\n");
	memset(&lchan, 0, sizeof(lchan));

	/* initialize the input. */
	for (i = 1; i < _MAX_SYSINFO_TYPE; ++i) {
		lchan.si.valid |= (1 << i);
		memset(&lchan.si.buf[i], i, sizeof(lchan.si.buf[i]));
	}

	/* It will start with '1' */
	for (i = 1, off = 0; i <= 32; ++i) {
		uint8_t *data = lchan_sacch_get(&lchan);
		off = (off + 1) % _MAX_SYSINFO_TYPE;
		if (off == 0)
			off += 1;

		//printf("i=%d (%%=%d) -> data[0]=%d\n", i, off, data[0]);
		OSMO_ASSERT(data[0] == off);
	}
}

int main(int argc, char **argv)
{
	bts_log_init(NULL);

	test_sacch_get();
	test_msg_utils_ipa();
	test_msg_utils_oml();
	return EXIT_SUCCESS;
}
