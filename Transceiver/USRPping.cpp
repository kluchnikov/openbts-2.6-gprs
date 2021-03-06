/*
* Copyright 2008, 2009 Free Software Foundation, Inc.
*
* This software is distributed under the terms of the GNU Public License.
* See the COPYING file in the main directory for details.
*
* This use of this software may be subject to additional restrictions.
* See the LEGAL file in the main directory for details.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/



#include <stdint.h>
#include <stdio.h>
#include "Transceiver.h"
#include <Logger.h>
#include <Configuration.h>

using namespace std;

ConfigurationTable gConfig;

int main(int argc, char *argv[]) {

  // Configure logger.
  if (argc>1) gLogInit(argv[1]);
  else gLogInit("INFO");
  if (argc>2) gSetLogFile(argv[2]);

  USRPDevice *usrp = new USRPDevice(400e3);

  usrp->make();

  TIMESTAMP timestamp;

  usrp->setTxFreq(890.0e6);
  usrp->setRxFreq(890.0e6);

  usrp->start();

  LOG(INFO) << "Looping...";
  bool underrun;

  //short data[]={0x00,0x02};

  usrp->updateAlignment(20000);
  usrp->updateAlignment(21000);

  int numpkts = 2;
  short data2[156*2*numpkts];
  for (int i = 0; i < 156*numpkts; i++) {
    data2[i<<1] = 10000;//4096*cos(2*3.14159*(i % 126)/126);
    data2[(i<<1) + 1] = 10000;//4096*sin(2*3.14159*(i % 126)/126);
  }

  for (int i = 0; i < 1; i++) 
    usrp->writeSamples((short*) data2,156*numpkts,&underrun,102000+i*1000);

  timestamp = 19000;
  while (1) {
    short readBuf[512*2];
    int rd = usrp->readSamples(readBuf,512,&underrun,timestamp);
    if (rd) {
      LOG(INFO) << "rcvd. data@:" << timestamp;
      /*for (int i = 0; i < 512; i++) {
        uint32_t *wordPtr = (uint32_t *) &readBuf[2*i];
        *wordPtr = usrp_to_host_u32(*wordPtr); 
	printf ("%d: %d %d\n", timestamp+i,readBuf[2*i],readBuf[2*i+1]);
      } */
      timestamp += rd;
    }
  }

}
