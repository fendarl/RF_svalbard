/*
 * main.cpp
 *
 *  Created on: Sep 4, 2018
 *      Author: Florian Anderl
 */


#include "../Usrp.h"
#include "../DFTNoise.h"
#include "../NoiseSpectrumHandler.h"
#include "../defines.h"


/*generic command line parser
 * DOCUMENTATION: -> https://github.com/jarro2783/cxxopts*/
#include <cxxopts.hpp>


uint64_t ext_num_FILE_recv_RF_samps;
uint64_t ext_num_INT_recv_RF_samps;
std::string ext_dev_addr;



/*Input Arguments: "Device Address, Start & End Frequency of frequency band of interest "*/

int main(int argc, char * argv[])
{

  /*temporary container for argc; required because cmlos.parse() overwrites argc (see below)*/
  int ARGC_T = argc;




  /*Command Line Parser*/

  cxxopts::Options cmlo("NoiseMapCPP", "UNIS Svalbard RF Background Recording (SuperDARN, KHO)");

  cmlo.add_options()
				("a, deviceAddress" , " USRP hardware address; if not defined default address (192.68.10.2) is used", cxxopts::value<std::string>())
				("l, lowerFrequency" , "Defines lower threshold for recorded band",cxxopts::value<uint64_t>())
				("u, upperFrequency","Defines upper threshold for recorded band ",cxxopts::value<uint64_t>())
				("g, gain", "Defines USRP Rx gain",cxxopts::value<int8_t>())
				;


  auto result = cmlo.parse(argc, argv);


  /*Sets threat priority*/
  uhd::set_thread_priority_safe();


  /*Default initialization of GLOBAL control variable*/
  ext_num_FILE_recv_RF_samps = 0;

  /*Default initialization of GLOBAL control variable*/
  ext_num_INT_recv_RF_samps = 0;



  /*Stores active RF mode*/
  RFmode activeRF_mode;


  /*------------------------- EXECUTION ----------------------------------------------*/


  Usrp *usrp_wrapper;


  /*----------------CMD ARGUMENT MODE--------------------------------------*/
  if(ARGC_T > 1)
    {
      activeRF_mode = RFmode::alternative_band;


      if((result.count("l") == 1) && (result.count("u") == 1))
	{

	  if(result.count("a") == 1)
	    {
	      usrp_wrapper = new Usrp(result["a"].as<std::string>(), result["l"].as<uint64_t>(),
				      result["u"].as<uint64_t>(), 0.0 ); /*TODO: rework gain*/
	    }

	  else
	    {
	      std::cout << "INFO: No USRP address defined -> using default address: 192.168.10.2 ";
	      usrp_wrapper = new Usrp(const_usrp_addr, result["l"].as<uint64_t>(),
				      result["u"].as<uint64_t>(), 0.0 ); /*TODO: rework gain*/
	    }

	}

      else
	{
	  std::cout << "ERROR: Please set frequency parameters" << std::endl;
	  return 0;
	}

    }




  /*-----------------DEFAULT MODE--------------------*/
  else
    {

      std::cout << "INFO: Using default configuration" << std::endl;
      activeRF_mode = RFmode::standard_band;
      usrp_wrapper = new Usrp();
    }









  /*-------------------------------------- STANDARD PROCEDURE -----------------------------------------------*/

  /*Sets USRP device parameters*/
  int conf_succ = usrp_wrapper->UsrpConfig();


  /*Wrapper Class for DFT Analysis*/
  DFTNoise *dft_wrapper = new DFTNoise();


  /*Container Variable storing start_time of */
  time_t init_start_time = time(NULL);



  NoiseSpectrumHandler *nsh = new NoiseSpectrumHandler(activeRF_mode);

  /*First Call of FileConfig -> but not the last (see while-loop below)
   * -> new call to FileConfig every 2h
   * -> see loop below */
  nsh->FileConfig(init_start_time);


  /*Initializes Receiver*/
  usrp_wrapper->UsrpStartUp();

  /*Temporary POINTER to Container of RF samples for hand-over to DFT*/
  std::complex<double> * temp_buff_rf;


  /*-------------------------RECEIVING SAMPLES & DATA PROCESSING -----------------------------*/

  while(true)
    {

      /*Issues Receive command to URSP */
      temp_buff_rf = usrp_wrapper->UsrpRFDataAcquisition();

      /*checks wether number of samples exceeds hard-coded threshold
       * -> if yes, new output file is created*/
      if(ext_num_FILE_recv_RF_samps > (DEF_SAMP_RATE * DEF_FILE_CONSTANT))
	{
	  /*reset ext_num_FILE_recv_RF_samps*/
	  ext_num_FILE_recv_RF_samps = 0;

	  delete nsh;  /*delete current instance of NSH*/

	  NoiseSpectrumHandler *nsh = new NoiseSpectrumHandler(activeRF_mode); /*create new instance of */

	  nsh->FileConfig(time(NULL));
	}


      /*Reset INTEGRATION variable every 60 seconds*/
      if(ext_num_INT_recv_RF_samps == (DEF_SAMP_RATE*60))
	{
	  ext_num_INT_recv_RF_samps = 0;
	}



      /*CONDITION:
       * IF received sample number is SMALLER OR EQUAL than the number of samples expected in defined period T
       * -> load sampls into buffer */
      if(ext_num_INT_recv_RF_samps <= (DEF_INTEGRATION_CONST * ext_sample_rate))
	{

	  dft_wrapper->GetRFSamples(temp_buff_rf);

	  dft_wrapper->ComputeNoiseDFT();

	  /*TODO: test & verify*/
	  nsh->GetDFTData(dft_wrapper->ExportDFTResults());


	  /*TODO: INVESTIGATE (Careful with BIGGER OR EQUALITY)!!!!!!*/
	  if((ext_num_INT_recv_RF_samps) >= (DEF_INTEGRATION_CONST * ext_sample_rate))
	    {
	      /*integrate DFT results TODO: go in function & fix it */
	      nsh->IntegrateDFT();

	      /*Converts DFT data: complex->magNitude*/
	      nsh->ConvertDFTData();

	      /*Exports Spectrum Data to file*/
	      nsh->ExportRawDataToFile();

	    }


	}

    }


  delete nsh;
  delete dft_wrapper;  /*free memory*/
  delete usrp_wrapper; /*free memory*/

  return conf_succ;
}


