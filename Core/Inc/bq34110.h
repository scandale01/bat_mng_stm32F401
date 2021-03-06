#pragma once
#include "stdint.h"
#include "i2c.h"

namespace bq34110 {
    constexpr int8_t selfAddress = 0xAA;

    struct eosLearnStatus {
      bool lcto; //Learn charge time out
      bool lfault; // Learn fault flag
      bool labrt; //Learn abort on command
      bool lcmd; //Learn command
      bool lper; // LEarn periodic mode
      bool lrlx; // Learn RELAX mode
      bool lchg; //Learn CHARGE mode
      bool ldsg; //Learn DISCHARGE mode
      bool ldone; //learn done
      bool lres; //Learned Rcell
      bool lrstor; // Learn Voltage Restore. This bit is set = 1 if whenever the Learn Discharge Phase is complete and the voltage can be
                  //restored back to its original level. This bit is cleared when the learning phase is complete.
      bool lctledge; //Learn Discharge Flag Edge Detected
      bool lucd; //Learn Unexpected Current Detected
      bool ldpam; //Learn Discharge Phase, Abort on Timer
      bool ldpat; //Learn Discharge Phase, Abort on Temperature
      bool ldpai; //Learn Discharge Phase, abort on current
    };

    namespace cmnd {
        constexpr uint8_t CNTRL = 0x00;       //0x00 and 0x01, unit NA (access R/W)
        constexpr uint8_t TEMP = 0x06;        //0x06 and 0x07, unit 0.1K (access R)
        constexpr uint8_t VOLT = 0x08;        //0x0A and 0x0B, unit NA (access R)
        constexpr uint8_t BSTAT = 0x0A;       //0x08 and 0x09, unit mV (access R)
        constexpr uint8_t Current = 0x0C;     //0x0C and 0x0D, unit mA (access R)
        constexpr uint8_t RC = 0x10;          //0x10 and 0x11, unit mAh (access R) Remaining Capacity
        constexpr uint8_t FCC = 0x12;         //0x12 and 0x13, unit mAh (access R) Full Charge Capacity
        constexpr uint8_t AI = 0x14;          //0x14 and 0x15, unit mA (access R) Avarage Current
        constexpr uint8_t TTE = 0x16;         //0x16 and 0x17, unit minutes (access R) Time To Empty
        constexpr uint8_t TTF = 0x18;         //0x18 and 0x19, unit minutes (access R) Time To Full
        constexpr uint8_t ACCCHAR = 0x1A;     //0x1A and 0x1B, unit mAh (access R) Accumulated Charge
        constexpr uint8_t ACCTIM = 0x1C;      //0x1C and 0x1D, unit 5min (access R) Accumulated Time
        constexpr uint8_t AP = 0x24;          //0x24 and 0x25, unit mW (access R) Average Power
        constexpr uint8_t INTTEMP = 0x28;     //0x28 and 0x29, unit 0.1K (access R) Internal Temperature 
        constexpr uint8_t CC = 0x2A;          //0x2A and 0x2B, unit number (access R) Cycle Count
        
        constexpr uint8_t RSOC = 0x2C;          //0x2C and 0x2D, unit % (access R) Relative State O Charge
        constexpr uint8_t SOH = 0x2E;           //0x2E and 0x2F, unit % (access R) State Of Health
        constexpr uint8_t CV = 0x30;            //0x30 and 0x31, unit mV (access R) Charging Voltage
        constexpr uint8_t CHC = 0x32;            //0x32 and 0x33, unit mA (access R) Charging Current
        constexpr uint8_t DESCAP = 0x3C;        //0x3C and 0x3D, unit mAh (access R) Design Capacity
        
        constexpr uint8_t MAC = 0x3E;           //0x3E and 0x3F, (access R/W) Manufacturer Access Control
        constexpr uint8_t MACDAT = 0x40;        //0x40 through 0x5F, (access R/W) MACData
        constexpr uint8_t MACDATSUM = 0x60;        //0x40 through 0x5F, (access R/W) MACData
        constexpr uint8_t MACDATLEN = 0x61;        //0x40 through 0x5F, (access R/W) MACData
        
        constexpr uint8_t EOSLERNSTAT = 0x64;   //0x64 and 0x65, (access R) EOSLearnStatus
        constexpr uint8_t EOSSTAT = 0x68;       //(access R) EOSStatus
        constexpr uint8_t ANALOG_COUNT = 0x79;    // returns the analog calibration counter
        constexpr uint8_t RAW_CURRENT = 0x7A;     //reads current at calibration
        constexpr uint8_t RAW_VOLTAGE = 0x7C;     //reads voltage at calibration
        constexpr uint8_t RAW_TEMP = 0x7E;        //reads temperature at calibration
    }

    namespace subcmnd{
         constexpr uint16_t CONTROL_STATUS = 0x0000;    //Reports the status of key features. This command should be 
                                                        // read back on 0x00; it will not read back on MACData().
         constexpr uint16_t DEVICE_TYPE = 0x0001;       //Reports the device type of 0x0110 (indicating the bq34110 device)
         constexpr uint16_t FW_VERSION = 0x0002;        //Reports the firmware version on the device type
         constexpr uint16_t BOARD_OFFSET = 0x0009;      //Invokes the board offset correction
         constexpr uint16_t CC_OFFSET = 0x000A;         //Invokes the CC offset correction
         constexpr uint16_t CC_OFFSET_SAVE = 0x000B;    //Saves the results of the offset calibration process
         constexpr uint16_t ACCUM_DSG_EN = 0x001E;      //Toggles the value of ManufacturingStatus()[ACDSG_EN]
         constexpr uint16_t ACCUM_CHG_EN = 0x001F;      //Toggles the value of ManufacturingStatus()[ACCHG_EN]
         
         constexpr uint16_t EOS_EN = 0x0021;            //Toggles the value of ManufacturingStatus()[EOS_EN]

         constexpr uint16_t CAL_TOGGLE = 0x002D;        //toggles the value of OperationStatus()[CALMD]

         constexpr uint16_t SECURITY_KEYS = 0x0035;     //Reads and writes Security Keys

         constexpr uint16_t EOS_START_LEARN = 0x0039;   //Initiates an EOS learning phase

         constexpr uint16_t EOS_RCELL_RRATE_LEARN = 0x003B;   //Initiates the Initial Rcell and Initial RRate learning procedures
         constexpr uint16_t EOS_WARNCLR = 0x003C;           //Clears the EOS Warning flags
         constexpr uint16_t EOS_INITIAL_RCELL = 0x003E;      //Used to read and write the EOS Initial Rcell value
         constexpr uint16_t EOS_INITIAL_RRATE = 0x003F;      //Used to read and write the EOS Initial RRate value
         constexpr uint16_t RESET = 0x0041;                 //Resets device
         constexpr uint16_t EOS_LOAD_ON = 0x0044;           //Turns on the learning load
         constexpr uint16_t EOS_LOAD_OFF = 0x0045;          //Turns off the learning load
         constexpr uint16_t DEVICE_NAME = 0x004A;          //This MAC subcommand returns the device name
         constexpr uint16_t ACCUM_RESET = 0x004B;          //Resets the Accumulated Charge integration counter
         constexpr uint16_t OPERAT_STATUS = 0x0054;          //This returns the same value as the OperationStatus() command
         constexpr uint16_t GAUGING_STATUS = 0x0056;        //This MAC subcommand returns the information in the CEDV gauging status register
         constexpr uint16_t MANUFACT_STATUS = 0x0057;       //This MAC subcommand returns the values of various functional modes of the device
    }

    class bq34 {
      public:
        struct batteryStatus {
          bool dsgIDetect;        //Discharge current detection
          bool chgIDetect;        //Charge current detection
          bool dsgAlarmTerminate; //Terminate Discharge Alarm,  flag is set and cleared based on the selected SOC Flag Config A options
          bool chgAlarmTerminate; //Terminate Charge Alarm, SOC Flag Config A options
          bool fullCharge;        //Full charge is detected ( bit is controlled by settings in SOC Flag Config A and SOC Flag Config B)
          bool fullDischarge;     //Full discharge is detected (based on the selected SOC Flag Config B options)
          bool chgInhibit;        //If set, this indicates charging should not begin because Temperature() is outside the acceptable range
          bool sleep;             //The device is in SLEEP mode when set.
          bool batLow;            //Battery Low Voltage Alarm (based on the selected thresholds in Safety.BATLOW data flash)
          bool batHigh;           // Battery High Voltage Alarm (based on the selected thresholds in Safety.BATLOW data flash)
          bool overTmpInDsgMode; // Overtemperature in Discharge condition is detected
          bool overTmpInChgMode; //Overtemperature in Charge condition is detected.
          bool lowTmpInDsgMode;
          bool lowTmpInChgMode;
          bool SOCLow;            //State-Of-Charge low detection
          bool testStarded;
        };

        struct batteryCondition {
          uint16_t voltage;
          uint16_t current;
          uint16_t remCap;
          uint16_t fullChgCap;
          uint32_t acummCharge;
        };

        struct sysParameters {
          uint8_t capScale ; //this scale % DOD voltage in 2 times
          uint8_t Voltage;  //default value for system voltage
          uint8_t CellNumber;  //default value for system voltage
          uint16_t Capacity; //17 Ah with a scale of 2 (capScale) 17/18/24/28/40/60 Ah
          uint8_t lowCapAlert_prct;
          uint8_t enExtTesting;
          uint16_t testCyclePeriod_days;
        };

        bq34();
        bool getStdCommandData(uint8_t cmnCode, uint16_t& data);
        bool getSubCommandData(uint8_t WCmnCode, uint16_t subCmnd, uint8_t RCmndCode, uint16_t& data);
        bool gaugeControlSubCmnd (uint16_t subCmnd);
        bool reset();
        bool gaugeReadDataClass(uint8_t subClass, uint8_t *pData, uint8_t dataLen);
        bool gaugeWriteDataClass(const uint16_t subClass, const uint8_t *pData, uint8_t dataLen);
        bool gaugeWriteDataClass(uint16_t subClass, int8_t *pData, uint8_t dataLen);
        bool operationConfigA();
        bool CEDVConfig();
        bool enterCalMode();
        bool exitCalMode();
        bool calibCCOffset();
        bool calibBoardOffset();
        bool calibRawCurr(uint16_t &currentVal);
        bool calibRawVoltage(uint16_t &voltagetVal);
        void updEOSLearnStatus();
        void getEOSStatus();
        void updBatStatus();
        void updBatCondData();
        bool isVoltNorm();
        uint32_t getVoltage();
        void startTest();
        void checkTestCondition(uint32_t& cntr);
        bool isTestStarted();
        batteryStatus m_batStatus;
        batteryCondition m_batCond;
        sysParameters m_sysData;
//        ~bq34();
      private:
        bool init();
        bool chargeInit();
        bool gaugeRead(uint8_t cmnd, uint8_t *pData, uint8_t dataLen);
        bool gaugeWrite(uint8_t *pData, uint8_t dataLen);
        bool unseal();
        eosLearnStatus m_EOSLernStatus;

    };

}
