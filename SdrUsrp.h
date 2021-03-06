/*
 * SdrUsrp.h
 *
 *  Created on: Sep 4, 2018
 *      Author: Florian Anderl
 */

#ifndef SDRUSRP_H_
#define SDRUSRP_H_

/*Standard Library*/
#include <string>
#include <vector>
#include <uhd/utils/thread.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/exception.hpp>
#include <uhd/types/tune_request.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <iostream>
#include "Constants.h"




class SdrUsrp {
public:
  SdrUsrp();
  virtual ~SdrUsrp();

  SdrUsrp(std::string usrp_addr, uint64_t l_freq, uint64_t u_freq, int8_t gain);

  int CalculateParameters();

  int PrepareSampleBuffer();

  int InitializeUSRP();

  int StartUpUSRP();


  std::complex<double> * RFDataAcquisitionUSRP();


private:

  uhd::usrp::multi_usrp::sptr  usrp_intern_;  /* intern USRP instance for which USRP class is wrapper*/
  uhd::rx_streamer::sptr rx_stream_; /* Receiver Streamer*/

  std::string usrp_address_; /* IP address of USRP; default = 192.168.10.2 */

  uint64_t center_frequency_;     /* LO oscillator/ DDC-discrete oscillator center frequency for Baseband-mixing*/
  uint64_t lower_frequency_;
  uint64_t upper_frequency_;

  uint64_t sample_rate_desired_;          /* target sample_rate that is sent to the PC from the FPGA, NB! Nyquist -> sample_rate determines the highest reproducible frequency*/

  int8_t gain_;                 /*Gain in dB of Rx chain*/


  RFmode usrp_mode_;


  /*Variables for Data Acquisition*/

  std::vector<std::complex<double>> buffs_;  /*Memory for received samples*/


  uhd::rx_metadata_t md_;	/*Container for Metadata or received samples*/



};

#endif /* SDRUSRP_H_ */
