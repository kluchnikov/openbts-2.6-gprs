/* GPRSL1Interface.cpp
 *
 * Copyright (C) 2012 Ivan Klyuchnikov
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */



#include <GSMLogicalChannel.h>
#include <GSMConfig.h>
#include <GSMTAPDump.h>
#include "GPRSL1Interface.h"

using namespace GSM;

UDPSocket RLCMACSocket;

void txPhConnectInd()
{
	char buffer[MAX_UDP_LENGTH] = {'\0'};
	int ofs = 0;
	struct gsm_pcu_if *prim = (struct gsm_pcu_if *)buffer;

	prim->msg_type = PCU_IF_MSG_INFO_IND;
	prim->bts_nr = 1;
	
	prim->u.info_ind.version = PCU_IF_VERSION;
	prim->u.info_ind.flags |= PCU_IF_FLAG_ACTIVE;
	prim->u.info_ind.flags |= PCU_IF_FLAG_CS1;
	prim->u.info_ind.trx[0].arfcn  = gConfig.getNum("GSM.ARFCN");
	std::vector<unsigned> ts = gConfig.getVector("GPRS.TS");
	uint8_t pdch_mask = 0;
	for (int i=0; i<ts.size(); i++) {
		pdch_mask = pdch_mask | (1 << ts[i]);
		prim->u.info_ind.trx[0].tsc[ts[i]] = gConfig.getNum("GSM.BCC");
	}
	prim->u.info_ind.trx[0].pdch_mask = pdch_mask;

	prim->u.info_ind.initial_cs = gConfig.getNum("GPRS.INITIAL_CS");

	prim->u.info_ind.t3142 = gConfig.getNum("GPRS.T3142");
	prim->u.info_ind.t3169 = gConfig.getNum("GPRS.T3169");
	prim->u.info_ind.t3191 = gConfig.getNum("GPRS.T3191");
	prim->u.info_ind.t3193_10ms = gConfig.getNum("GPRS.T3193_10MS");
	prim->u.info_ind.t3195 = gConfig.getNum("GPRS.T3195");
	prim->u.info_ind.n3101 = gConfig.getNum("GPRS.T3101");
	prim->u.info_ind.n3103 = gConfig.getNum("GPRS.T3103");
	prim->u.info_ind.n3105 = gConfig.getNum("GPRS.T3105");
	
	/* RAI */
	prim->u.info_ind.bsic = (gConfig.getNum("GSM.NCC") << 3) | gConfig.getNum("GSM.BCC");
	prim->u.info_ind.mcc = strtol(gConfig.getStr("GPRS.MCC"), NULL, 16);
	prim->u.info_ind.mnc = strtol(gConfig.getStr("GPRS.MNC"), NULL, 16);
	prim->u.info_ind.lac = gConfig.getNum("GSM.LAC");
	prim->u.info_ind.rac = gConfig.getNum("GPRS.RAC");
	/* NSE */
	prim->u.info_ind.nsei = gConfig.getNum("GPRS.NSEI");
	/* cell */
	prim->u.info_ind.cell_id = gConfig.getNum("GPRS.CELL_ID");
	prim->u.info_ind.repeat_time = gConfig.getNum("GPRS.REPEAT_TIME");
	prim->u.info_ind.repeat_count = gConfig.getNum("GPRS.REPEAT_COUNT");
	prim->u.info_ind.bvci = gConfig.getNum("GPRS.BVCI");

	prim->u.info_ind.cv_countdown = gConfig.getNum("GPRS.CV_COUNTDOWN");
	/* NSVC */
	prim->u.info_ind.nsvci[0] = gConfig.getNum("GPRS.NSVCI");
	prim->u.info_ind.local_port[0] = gConfig.getNum("GPRS.NSVC_LPORT");
	prim->u.info_ind.remote_port[0] = gConfig.getNum("GPRS.NSVC_RPORT");
	
	struct sockaddr_in dest;
	dest.sin_family = AF_INET;
	dest.sin_port = htons(gConfig.getNum("GPRS.NSVC_RPORT"));
	inet_aton(gConfig.getStr("GPRS.SGSN.IP"), &dest.sin_addr);
	prim->u.info_ind.remote_ip[0] = ntohl(dest.sin_addr.s_addr);

	ofs = sizeof(*prim);

	LOG(NOTICE) << "TX:[BTS->PCU] PhConnectInd: ARFCN:" << gConfig.getNum("GSM.ARFCN")
		<<" TN:" << gConfig.getStr("GPRS.TS") << " TSC:" << gConfig.getNum("GSM.BCC");
	RLCMACSocket.write(buffer, ofs);
}

void GPRS::txPhRaInd(unsigned ra, int Fn, unsigned ta)
{
	char buffer[MAX_UDP_LENGTH];
	int ofs = 0;
	struct gsm_pcu_if *prim = (struct gsm_pcu_if *)buffer;

	prim->msg_type = PCU_IF_MSG_RACH_IND;
	prim->bts_nr = 1;
	prim->u.rach_ind.sapi = PCU_IF_SAPI_RACH;
	prim->u.rach_ind.ra = ra;
	prim->u.rach_ind.qta = ta;
	prim->u.rach_ind.fn = Fn;
	prim->u.rach_ind.arfcn = gConfig.getNum("GSM.ARFCN");
	ofs = sizeof(*prim);

	LOG(NOTICE) << "TX:[BTS->PCU] PhRaInd: RA:" << ra <<" FN:" << Fn << " TA:" << ta;
	RLCMACSocket.write(buffer, ofs);
}

void txMphTimeInd()
{
	char buffer[MAX_UDP_LENGTH];
	int ofs = 0;
	struct gsm_pcu_if *prim = (struct gsm_pcu_if *)buffer;

	prim->msg_type = PCU_IF_MSG_TIME_IND;
	prim->bts_nr = 1;
	prim->u.time_ind.fn = gBTS.time().FN();
	ofs = sizeof(*prim);

	LOG(DEBUG) <<"TX:[BTS->PCU] MphTimeInd FN:" << prim->u.time_ind.fn;
	RLCMACSocket.write(buffer, ofs);
}

void GPRS::txPhReadyToSendInd(unsigned Tn, int Fn)
{
	char buffer[MAX_UDP_LENGTH];
	int ofs = 0;
	struct gsm_pcu_if *prim = (struct gsm_pcu_if *)buffer;

	prim->msg_type = PCU_IF_MSG_RTS_REQ;
	prim->bts_nr = 1;
	prim->u.rts_req.sapi = PCU_IF_SAPI_PDTCH;
	prim->u.rts_req.fn = Fn;
	prim->u.rts_req.arfcn = gConfig.getNum("GSM.ARFCN");
	prim->u.rts_req.trx_nr = 0;
	prim->u.rts_req.ts_nr = Tn;
	int bn = 0;

	int fn52 = 4;
	while(fn52 < 52)
	{
		if ((Fn%52)<fn52)
		{
			break;
		}
		else
		{
			bn++;
			fn52 += 4;
		}
	}

	prim->u.rts_req.block_nr = bn;

	ofs = sizeof(*prim);

	LOG(DEBUG) << "TX:[BTS->PCU] PhReadyToSendInd: TS:" << Tn << " FN:" << Fn;
	RLCMACSocket.write(buffer, ofs);
	txMphTimeInd();
}



void GPRS::txPhDataInd(const RLCMACFrame *frame, GSM::Time readTime,
										 unsigned ts_nr, float rssi)
{
	char buffer[MAX_UDP_LENGTH];
	int ofs = 0;
	struct gsm_pcu_if *prim = (struct gsm_pcu_if *)buffer;

	prim->msg_type = PCU_IF_MSG_DATA_IND;
	prim->bts_nr = 1;
	
	prim->u.data_ind.sapi = PCU_IF_SAPI_PDTCH;
	frame->pack((unsigned char*)&(prim->u.data_ind.data[ofs]));
	ofs += frame->size() >> 3;
	prim->u.data_ind.len = ofs;
	prim->u.data_ind.arfcn = gConfig.getNum("GSM.ARFCN");
	int Fn = readTime.FN();
	prim->u.data_ind.fn = Fn;
	int bn = 0;

	int fn52 = 4;
	while(fn52 < 52)
	{
		if ((Fn%52)<fn52)
		{
			break;
		}
		else
		{
			bn++;
			fn52 += 4;
		}
	}

	prim->u.data_ind.block_nr = bn;
	prim->u.data_ind.trx_nr = 0;
	prim->u.data_ind.ts_nr = ts_nr;
	prim->u.data_ind.rssi = (int)rssi;

	ofs = sizeof(*prim);

	LOG(NOTICE) << "TX:[BTS->PCU ] PhDataInd TS:" << ts_nr
				<< " FN:" << Fn << " RSSI:" << (int)rssi << " DATA:" << *frame;
	//gWriteGSMTAP(gConfig.getNum("GSM.Radio.C0"), ts_nr, Fn,
	//							GSM::PDCH, false, true, *frame);
	RLCMACSocket.write(buffer, ofs);
	txMphTimeInd();
}

void txPhDataIndCnf(const BitVector &frame, GSM::Time readTime)
{
	char buffer[MAX_UDP_LENGTH];
	int ofs = 0;
	struct gsm_pcu_if *prim = (struct gsm_pcu_if *)buffer;

	prim->msg_type = PCU_IF_MSG_DATA_CNF;
	prim->bts_nr = 1;
	
	prim->u.data_ind.sapi = PCU_IF_SAPI_PCH;
	frame.pack((unsigned char*)&(prim->u.data_ind.data[ofs]));
	ofs += frame.size() >> 3;
	prim->u.data_ind.len = ofs;
	prim->u.data_ind.arfcn = gConfig.getNum("GSM.ARFCN");
	int Fn = readTime.FN();
	prim->u.data_ind.fn = Fn;
	int bn = 0;

	int fn52 = 4;
	while(fn52 < 52)
	{
		if ((Fn%52)<fn52)
		{
			break;
		}
		else
		{
			bn++;
			fn52 += 4;
		}
	}

	prim->u.data_ind.block_nr = bn;
	prim->u.data_ind.trx_nr = 0;
	prim->u.data_ind.ts_nr = 0;

	ofs = sizeof(*prim);

	LOG(NOTICE) << "TX:[BTS->PCU] PhDataIndCnf TS:"
				<< prim->u.data_ind.ts_nr << " FN:" << Fn;
	RLCMACSocket.write(buffer, ofs);
	txMphTimeInd();
}

BitVector* readL1Prim(unsigned char* buffer, int *sapi, int *ts, int *fn)
{
	struct gsm_pcu_if *prim = (struct gsm_pcu_if *)buffer;

	switch(prim->msg_type)
	case PCU_IF_MSG_DATA_REQ:
	{
		*sapi = prim->u.data_req.sapi;
		*ts = prim->u.data_req.ts_nr;
		*fn = prim->u.data_req.fn;
		BitVector * msg = new BitVector(prim->u.data_req.len*8);
		msg->unpack((const unsigned char*)prim->u.data_req.data);
		return msg;
	}

	return NULL;
}

void GPRS::GPRSReader(LogicalChannel **PDCH)
{
	RLCMACSocket.open(gConfig.getNum("GPRS.Local.Port"));
	RLCMACSocket.destination(gConfig.getNum("GPRS.PCU.Port"),
						gConfig.getStr("GPRS.PCU.IP"));

	RLCMACSocket.nonblocking();

	char buf[MAX_UDP_LENGTH];

	int l2Len = 0;

	// Send to PCU PhConnectInd primitive.
	
	txPhConnectInd();

	while (1)
	{
		int count = RLCMACSocket.read(buf, 3000);
		if (count>0)
		{
			int sapi;
			int ts, fn;
			BitVector *msg = readL1Prim((unsigned char*) buf, &sapi, &ts, &fn);
			if (!msg)
			{
				delete msg;
				continue;
			}
			if (sapi == PCU_IF_SAPI_PDTCH)
			{
				RLCMACFrame *frame = new RLCMACFrame(*msg);
				frame->fn(fn);
				// write dummy downlink control message to LOG only in DEBUG mode
				if (frame->peekField(0,16) == 0x4794)
				{
					LOG(DEBUG) << "RX:[BTS<-PCU] PhDataReq TS:" << ts
								 << " FN:" << fn << " DATA:" << *frame;
				}
				else
				{
					LOG(NOTICE) << "RX:[BTS<-PCU] PhDataReq TS:" << ts
								 << " FN:" << fn << " DATA:" << *frame;
				}
				((PDTCHLogicalChannel*)PDCH[ts])->sendRLCMAC(frame);
			}
			else if (sapi == PCU_IF_SAPI_AGCH)
			{
				// Get an AGCH to send on.
				CCCHLogicalChannel *AGCH = gBTS.getAGCH();
				// Check AGCH load now.
				if ((!AGCH)||(AGCH->load()>(unsigned)gConfig.getNum("GSM.AGCH.QMax")))
				{
					LOG(NOTICE) << "GPRS AGCH congestion";
					delete msg;
					continue;
				}
				L3Frame *l3 = new L3Frame(msg->tail(8), UNIT_DATA);
				LOG(NOTICE) << "RX:[BTS<-PCU] AGCH:" << *l3;
				l2Len = msg->peekField(0, 6);
				l3->L2Length(l2Len);
				AGCH->send(l3);
				txPhDataIndCnf(*msg, gBTS.time());
			}
			else if (sapi == PCU_IF_SAPI_PCH)
			{
				L3Frame *msg1 = new L3Frame(msg->tail(8*4), UNIT_DATA);
				L3Frame *msg2 = new L3Frame(msg->tail(8*4), UNIT_DATA);
				LOG(NOTICE) << "RX:[BTS<-PCU] PCH:" << *msg1;
				l2Len = msg->peekField(8*3, 6);
				msg1->L2Length(l2Len);
				msg2->L2Length(l2Len);
				// HACK -- We send every page twice.
				gBTS.getPCH(0)->send(msg1);
				gBTS.getPCH(0)->send(msg2);
				txPhDataIndCnf(msg->tail(8*3), gBTS.time());
			}
			delete msg;
		}
	}
}

