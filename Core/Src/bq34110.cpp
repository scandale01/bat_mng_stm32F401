#include "bq34110.h"
#include "adc.h"
#include "stdint.h"
#include "tim.h"
#include "i2c.h"
#include "stdio.h"
#include <bitset>

namespace  {
  int16_t voltageDOD[10] = {2100, 2083, 2070, 2053, 2033, 2010,
      1983, 1958, 1930, 1885
  };
  struct batteryStatus {
    bool dsgIDetect; //Discharge current detection
    bool chgIDetect; //Charge current detection
    bool dsgAlarmTerminate; //Terminate Discharge Alarm,  flag is set and cleared based on the selected SOC Flag Config A options
    bool chgAlarmTerminate; //Terminate Charge Alarm, SOC Flag Config A options
    bool fullCharge; //Full charge is detected ( bit is controlled by settings in SOC Flag Config A and SOC Flag Config B)
    bool fullDischarge; //Full discharge is detected (based on the selected SOC Flag Config B options)
    bool chgInhibit; //If set, this indicates charging should not begin because Temperature() is outside the acceptable range
    bool sleep; //The device is in SLEEP mode when set.
    bool batLow; //Battery Low Voltage Alarm (based on the selected thresholds in Safety.BATLOW data flash)
    bool batHigh; // Battery High Voltage Alarm (based on the selected thresholds in Safety.BATLOW data flash)
    bool overTmpInDsgMode; // Overtemperature in Discharge condition is detected
    bool overTmpInChgMode; //Overtemperature in Charge condition is detected.
    bool lowTmpInDsgMode;
    bool lowTmpInChgMode;
    bool SOCLow;          //State-Of-Charge low detection
  };

}

namespace bq34110 {

    bq34::bq34() {
      m_sysData.capScale = 2; //this scale % DOD voltage in 2 times
      m_sysData.Voltage = 24;  //default value for system voltage
      m_sysData.CellNumber = 12;  //default value for system voltage
      m_sysData.Capacity = 8500; //17 Ah with a scale of 2 (capScale) 17/18/24/28/40/60 Ah
      m_sysData.testCyclePeriod_days = 60 ; //60 days test period
      m_sysData.lowCapAlert_prct = 40;

      m_batStatus.testStarded = false;

      HAL_ADC_Start(&hadc1);

      while(HAL_ADC_PollForConversion(&hadc1, 10000)!= HAL_OK);
      uint32_t adcVal = HAL_ADC_GetValue(&hadc1);
      float voltage = (3.3f*adcVal / 4095) * (6000/(6000 + 54000));
      if(voltage > 40){
        m_sysData.Voltage = 48;
        m_sysData.CellNumber = 12 + 12;
        HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 500);
      }
      //Read GPIO for configuation setup
      /*
      if (HAL_GPIO_ReadPin(GPIOx, GPIO_Pin)) {
        m_sysData.Capacity = 9000; // nex st
      }
      if (HAL_GPIO_ReadPin(GPIOx, GPIO_Pin)) {
        //set periodic timer for testing
        //or read if pin external pin is high (activate some flag)
      }
      */
      unseal();
      init();
      gaugeControlSubCmnd(bq34110::subcmnd::EOS_LOAD_OFF);
    }

    bool bq34::getStdCommandData(uint8_t cmnCode, uint16_t& data)
    {
        uint8_t data_tmp[2];
        if(!(this->gaugeRead(cmnCode, data_tmp, sizeof(data_tmp))))
        {
          return false;
        } else {
          data = data_tmp[0] | (data_tmp[1] << 8);
          return true;
        }
    }

    bool bq34::getSubCommandData(uint8_t WCmnCode, uint16_t subCmnd, uint8_t RCmndCode, uint16_t& data)
    {
        uint8_t i2c_data[3];
        i2c_data[0] = WCmnCode;
        i2c_data[1] = subCmnd;
        i2c_data[2] = 0xFF & subCmnd >> 8;
        if(HAL_I2C_Master_Transmit(&hi2c1, static_cast<uint16_t>(bq34110::selfAddress), (uint8_t*)i2c_data, sizeof(i2c_data), 100) != HAL_OK)
          return false;
        HAL_Delay(1500);
        return this->getStdCommandData(RCmndCode, data);
    }

    bool bq34::gaugeControlSubCmnd (uint16_t subCmnd)
    {
      uint8_t data[3];
      data[0] = bq34110::cmnd::CNTRL;
      data[1] = subCmnd;
      data[2] = 0xFF & subCmnd >> 8;
      if (!this->gaugeWrite(data, sizeof(data))) {
        return false;
      }
      return true;
    }

    //sets device is SEALED mode after reset
    bool bq34::reset()
    {
      uint8_t i2c_data[3];
      i2c_data[0] = bq34110::cmnd::CNTRL;
      i2c_data[1] = bq34110::subcmnd::RESET;
      i2c_data[2] = 0xFF & bq34110::subcmnd::RESET >> 8;
      if(!this->gaugeWrite(i2c_data, sizeof(i2c_data)))
      {
        return false;
      } else
        return true;
    }

    /*
     * Read the results from a gauge standard command.
     * subClass - Subclass
     * pData -  holds returned data from the gauge
     */
    bool bq34::gaugeReadDataClass(uint8_t subClass, uint8_t *pData, uint8_t dataLen)
    {
      uint8_t i2c_data[3];
      i2c_data[0] = bq34110::cmnd::MAC;
      i2c_data[1] = subClass;
      i2c_data[2] = 0xFF & subClass >>8;
      if (!this->gaugeWrite(i2c_data, sizeof(i2c_data))) {
        return false;
      }
      HAL_Delay(1500);
      if (this->gaugeRead(bq34110::cmnd::MACDAT, pData, dataLen)) {
        return false;
      }else
        return true;
    }

    /*
     *  Write data for a gauge standard command, for exmaple: change design capacity and design energy.
     * subClass - Subclass
     * pData -  holds returned data from the gauge
     *
     * To change parameters, first identify the data class (also called subclass) from the TRM.
     */
    bool bq34::gaugeWriteDataClass(const uint16_t subClass, const uint8_t *pData, uint8_t dataLen)
    {
      uint8_t *data = new uint8_t[3 + dataLen];
      data[0] = bq34110::cmnd::MAC;
      data[1] = subClass;
      data[2] = 0xFF & subClass >>8;
      for (uint8_t var = 0;  var < dataLen; var++) {
        data[3 + var] = *(pData + var);
      }
      if (! this->gaugeWrite(data, sizeof(data))) {
        return false;
      }
      uint8_t dataCheckSum[2];
      dataCheckSum[0] = bq34110::cmnd::MACDATSUM;
      dataCheckSum[1] = 0;
      for (uint8_t var = 0;  var < dataLen; var++) {
        dataCheckSum[1] += *(pData + var);
      }
      dataCheckSum[1] = ~((dataCheckSum[1] + data[1] + data[2]) & 0xFF);
      if(!this->gaugeWrite(dataCheckSum, sizeof(dataCheckSum)))
        return false;
      dataCheckSum[0] = bq34110::cmnd::MACDATLEN;
      dataCheckSum[1] = 4 + dataLen;          //4 + sizeof write config bytes
      if(!this->gaugeWrite(dataCheckSum, sizeof(dataCheckSum)))
        return false;
      delete[] data;
      return true;
    }
//    bool bq34::gaugeWriteDataClass(uint16_t subClass, int8_t *pData, uint8_t dataLen)
//  {
//    uint8_t *data = new uint8_t[3 + dataLen];
//    data[0] = bq34110::cmnd::MAC;
//    data[1] = subClass;
//    data[2] = 0xFF & subClass >>8;
//    for (uint8_t var = 0;  var < dataLen; var++) {
//      data[3 + var] = *(pData + var);
//    }
//    if (! this->gaugeWrite(data, sizeof(data))) {
//      return false;
//    }
//    uint8_t dataCheckSum[2];
//    dataCheckSum[0] = bq34110::cmnd::MACDATSUM;
//    dataCheckSum[1] = 0;
//    for (uint8_t var = 0;  var < dataLen; var++) {
//      dataCheckSum[1] += *(pData + var);
//    }
//    dataCheckSum[1] = ~((dataCheckSum[1] + data[1] + data[2]) & 0xFF);
//    if(!this->gaugeWrite(dataCheckSum, sizeof(dataCheckSum)))
//      return false;
//    dataCheckSum[0] = bq34110::cmnd::MACDATLEN;
//    dataCheckSum[1] = 4 + dataLen;          //4 + sizeof write config bytes
//    if(!this->gaugeWrite(dataCheckSum, sizeof(dataCheckSum)))
//      return false;
//    delete[] data;
//    return true;
//  }

    bool bq34::operationConfigA()
    {
      uint16_t configAdr = 0x413A;
      uint8_t i2c_data[5];
      i2c_data[0] = bq34110::cmnd::MAC;
      i2c_data[1] = configAdr;
      i2c_data[2] = 0xFF & configAdr >> 8;
      /*
       * Write data is BigEndian format
       * SCALED = 1, SLEEP = 1, JEITA = 1
       */
      i2c_data[3] = 0x06;    //confgih High byte
      i2c_data[4] = 0x80;   //config Low byte
      if(!this->gaugeWrite(i2c_data, sizeof(i2c_data)))
        return false;
      /*
       * Separately MACDataSum and MACDataLen write after
       */
      uint8_t dataCheckSum[2];
      dataCheckSum[0] = bq34110::cmnd::MACDATSUM;
      dataCheckSum[1] = ~((i2c_data[1] + i2c_data[2] + i2c_data[3] + i2c_data[4]) & 0xFF);
      if(!this->gaugeWrite(dataCheckSum, sizeof(dataCheckSum)))
        return false;
      dataCheckSum[0] = bq34110::cmnd::MACDATLEN;
      dataCheckSum[1] = 4 + 2;          //4 + sizeof write config bytes
      if(!this->gaugeWrite(dataCheckSum, sizeof(dataCheckSum)))
        return false;
      return true;
    }

    bool bq34::CEDVConfig()
    {
      uint16_t configAdr = 0x424B;
      uint8_t i2c_data[2];

      i2c_data[0] = 0x24;    //confgih High byte
      i2c_data[1] = 0x1a ;   //config Low byte
      if(!this->gaugeWriteDataClass(configAdr, i2c_data, sizeof(i2c_data)))
        return false;
      return true;
    }

    bool bq34::enterCalMode()
    {
      bool status = false;
      uint16_t readData;
      while(!status) {
       // Calibration toggle command
        if (!this->gaugeControlSubCmnd(bq34110::subcmnd::CAL_TOGGLE)) {
          status = false;
        }
        if (!this->getSubCommandData(bq34110::cmnd::CNTRL, bq34110::subcmnd::MANUFACT_STATUS, bq34110::cmnd::MACDAT, readData)) {
          status = false;
        }
        if(readData & (1 << 15))
        {
          status = true;
        }
      }
      return true;
    }
    bool bq34::exitCalMode()
    {
      bool status = false;
      uint16_t readData;
      while(!status) {
        if(!this->gaugeControlSubCmnd(bq34110::subcmnd::CAL_TOGGLE)) {
          status = false;
        }
        if (!this->getSubCommandData(bq34110::cmnd::CNTRL, bq34110::subcmnd::MANUFACT_STATUS, bq34110::cmnd::MACDAT, readData)) {
          status = false;
        }
        if(readData & (0 << 15)) {
          status = true;
        }
      }
      return true;
    }

    bool bq34::calibCCOffset()
    {
      bool status = false;
      uint16_t readData;
      while(!status) {
        if (!this->gaugeControlSubCmnd(bq34110::subcmnd::CC_OFFSET)) {
          status = false;
        }
        if (!this->getSubCommandData(bq34110::cmnd::CNTRL, bq34110::subcmnd::CONTROL_STATUS, bq34110::cmnd::CNTRL, readData)) {
          status = false;
        }
        //if CCA == 1, calibration started
        if(readData & (1 << 5)) {
          //while CCA != 0 it is calibrating, wait for clear
          while(!(readData & (0 << 5))) {
            this->getSubCommandData(bq34110::cmnd::CNTRL, bq34110::subcmnd::CONTROL_STATUS, bq34110::cmnd::CNTRL, readData);
            HAL_Delay(500);
          }
        }
      }
      return true;
    }

    /*
     * Calculating the Board Offset also calculates the CC Offset; therefore, it is not necessary to go through
     * the CC Offset calibration process if the Board Offset calibration process is implemented.
     */
    bool bq34::calibBoardOffset()
    {
      bool status = false;
      uint16_t readData;
      while(!status) {
        if (!this->gaugeControlSubCmnd(bq34110::subcmnd::BOARD_OFFSET)) {
          status = false;
        }
        if (!this->getSubCommandData(bq34110::cmnd::CNTRL, bq34110::subcmnd::CONTROL_STATUS, bq34110::cmnd::CNTRL, readData)) {
          status = false;
        }
        //if CCA == 1, calibration started
        if((readData & (1 << 4)) && (readData & (1 << 5))) {
          //while CCA & BOARD != 0 it is calibrating, wait for clear
          while(!(readData & (0 << 4))) {
            this->getSubCommandData(bq34110::cmnd::CNTRL, bq34110::subcmnd::CONTROL_STATUS, bq34110::cmnd::CNTRL, readData);
            HAL_Delay(500);
          }
        }
      }
      return true;
    }

    bool bq34::calibRawCurr(uint16_t &currentVal)
    {
      uint16_t loopCount = 0;
      uint16_t rawDataSum = 0;
      uint8_t counterNow;
      uint8_t counterPrev;
      //10 samples for averaging
      for (; loopCount < 10; ++loopCount) {
        if(!this->gaugeRead(bq34110::cmnd::ANALOG_COUNT, &counterNow, sizeof(counterNow))) {
          return false;
        }
        counterPrev = counterNow;
        HAL_Delay(200);
        if (!this->gaugeRead(bq34110::cmnd::ANALOG_COUNT, &counterNow, sizeof(counterNow))) {
          return false;
        }
        while(counterNow == counterPrev) {
          HAL_Delay(200);
          this->gaugeRead(bq34110::cmnd::ANALOG_COUNT, &counterNow, sizeof(counterNow));
        }
        uint16_t tempCurr;
        if(!this->getStdCommandData(bq34110::cmnd::RAW_CURRENT, tempCurr))
          return false;
        rawDataSum += tempCurr;
        counterPrev = counterNow;
      }
      currentVal = (rawDataSum / 10) / m_sysData.capScale; //@TODO smth wrong there, capScale ??
      return true;
    }

    bool bq34::calibRawVoltage(uint16_t &voltagetVal)
    {
      uint16_t loopCount = 0;
      uint16_t rawDataSum = 0;
      uint8_t counterNow;
      uint8_t counterPrev;
      //10 samples for averaging
      for (; loopCount < 10; ++loopCount) {
        if(!this->gaugeRead(bq34110::cmnd::ANALOG_COUNT, &counterNow, sizeof(counterNow))) {
          return false;
        }
        counterPrev = counterNow;
        HAL_Delay(200);
        if (!this->gaugeRead(bq34110::cmnd::ANALOG_COUNT, &counterNow, sizeof(counterNow))) {
          return false;
        }
        while(counterNow == counterPrev) {
          HAL_Delay(200);
          this->gaugeRead(bq34110::cmnd::ANALOG_COUNT, &counterNow, sizeof(counterNow));
        }
        uint16_t tempVolt;
        if(!this->getStdCommandData(bq34110::cmnd::RAW_VOLTAGE, tempVolt))
          return false;
        rawDataSum += tempVolt;
        counterPrev = counterNow;
      }
      voltagetVal = (rawDataSum / 10);
      return true;
    }

    bool bq34::gaugeRead(uint8_t cmnd, uint8_t *pData, uint8_t dataLen)
    {
      uint8_t cmndDat= cmnd;
      if (HAL_I2C_Master_Transmit(&hi2c1, static_cast<uint16_t>(bq34110::selfAddress), (uint8_t*)&cmndDat, sizeof(cmnd), 500) != HAL_OK) {
        return false;
      }
      if (HAL_I2C_Master_Receive(&hi2c1, static_cast<uint16_t>(bq34110::selfAddress), pData, dataLen, 800) != HAL_OK) {
        return false;
      } else
        return true;
    }

    bool bq34::gaugeWrite(uint8_t *pData, uint8_t dataLen)
    {
      if (HAL_I2C_Master_Transmit(&hi2c1, static_cast<uint16_t>(bq34110::selfAddress), (uint8_t*)pData, dataLen, 500) != HAL_OK)
      {
        return false;
      } else {
        HAL_Delay(1);
        return true;
      }
    }

    bool bq34::unseal() {
//      if (!gaugeControlSubCmnd(bq34110::subcmnd::SECURITY_KEYS)) {
//        return false;
//      }
//      HAL_Delay(1);
      uint8_t data[3];
      uint8_t keyArr[4] = {0x36, 0x72, 0x04, 0x14};
      data[0] = bq34110::cmnd::CNTRL;
      data[1] = keyArr[3];
      data[2] = keyArr[2];
//      data[3] = keyArr[1];
//      data[4] = keyArr[0];
      if (!gaugeWrite(data, sizeof(data))) {
        return false;
      }
      data[1] = keyArr[1];
      data[2] = keyArr[0];
      HAL_Delay(1);
      if (!gaugeWrite(data, sizeof(data))) {
        return false;
      }
//      uint8_t dataCheckSum[2];
//      dataCheckSum[0] = bq34110::cmnd::MACDATSUM;
//      dataCheckSum[1] = ~((bq34110::subcmnd::SECURITY_KEYS +
//          data[1] + data[2] + data[3] + data[4]) & 0xFF);
//      if(!gaugeWrite(dataCheckSum, sizeof(dataCheckSum)))
//        return false;
//      dataCheckSum[0] = bq34110::cmnd::MACDATLEN;
//      dataCheckSum[1] = 4 + 4;          //4 + sizeof write config bytes
//      if(!this->gaugeWrite(dataCheckSum, sizeof(dataCheckSum)))
//        return false;
      return true;
    }

    bool bq34::init()
    {
      /*
       * Settings class configuring
       * DCHGPOL = 1 && DCP2 = 1
       * If set, the pin selected for direct charger control will generate a 1 output when charging is
       * to be enabled
       *  ALERT1 pin is used for direct charger control
       */
      uint8_t chargePinConfig = 0x0C;
      if(!this->gaugeWriteDataClass(0x4195, &chargePinConfig, sizeof(chargePinConfig)))
        return false;

      /*
       * VEN_EN = 1 for external voltage divider operation
       * ALERT IS NOT CONFIGURED, BUT IT CAN BE USED
       */
      uint8_t pinControlConfig = 0x10;
      if(!this->gaugeWriteDataClass(0x413D, &pinControlConfig, sizeof(pinControlConfig)))
        return false;

      /*
       * Config for initial capacity
       * CEDV Profile -> Design Capacity mAh
       * This is intended to be the theoretical
       * or nominal capacity of a new pack in units of 1 mAh per LSB, and is
       * used for the calculation of StateOfHealth().
       */
      uint8_t bqDesignCapacity[2];
      bqDesignCapacity[0] = (m_sysData.Capacity/2) >> 8; //High byte first
      bqDesignCapacity[1] = m_sysData.Capacity/2;
      if(!this->gaugeWriteDataClass(0x41F5, bqDesignCapacity, sizeof(bqDesignCapacity)))
        return false;

      /*
       * Learned Full Charge Capacity
       * Gas Gauging -> State Profile > Learned Full Charge Capacity mAh
       */
      uint8_t bqLearnFullChargeCapacity[2];
      bqDesignCapacity[0] = (m_sysData.Capacity/2) >> 8; //High byte first
      bqDesignCapacity[1] = m_sysData.Capacity/2;
      if(!this->gaugeWriteDataClass(0x40C0, bqLearnFullChargeCapacity, sizeof(bqDesignCapacity)))
        return false;

      /*
       * Config. for battery status flag setings
       * FCSETVCT = 1  Enables BatteryStatus()[FC] flag set on primary charge termination
       * TCSETV = 1 Enables BatteryStatus()[TCA] flag set when Voltage() ≥ TC:Set Voltage Threshold
       * TDCLEARV = 1 Enables BatteryStatus()[TDA] flag clear when Voltage() ≥ TD:Clear Voltage Threshold
       * TDSETV = 1 Enables BatteryStatus()[TDA] flag set when Voltage() ≤ TD:Set Voltage Threshold
       * socFlagConfA = 0x0413;
       */
      uint8_t socFlagConfA[2];
      socFlagConfA[0] = 0x04; //High byte first
      socFlagConfA[1] = 0x13;
      if(!this->gaugeWriteDataClass(0x41FD, socFlagConfA, sizeof(socFlagConfA)))
        return false;

      /*
       * FCCLEARV = 1 Enables BatteryStatus()[FC] flag clear when Voltage() ≤ FC:Clear Voltage Threshold
       * FCSETV = 1 Enables BatteryStatus()[FC] flag set when Voltage() ≥ FC:Set Voltage Threshold
       * FDCLEARV = 1 Enables BatteryStatus()[FD] flag clear when Voltage() ≥ FD:Clear Voltage Threshold
       * FDSETV = 1 Enables BatteryStatus()[FD] flag set when Voltage() ≤ FD:Set Voltage Threshold
       */
      uint8_t socFlagConfB = 0x33;
      if(!this->gaugeWriteDataClass(0x41FF, &socFlagConfB, sizeof(socFlagConfB)))
        return false;
      /*
       * LENCTL = 1 -  The LEN pin is used to control Learn Discharge Current.
       * LSM = 1 -  DISCHARGE-BEFORE-CHARGE mode is used.
       * LVR = 0 - The device stops Learn Discharge Phase when the timer reaches Learn Discharge Time
       */
      uint8_t EOSConfiguration = 0x03;
      if(!this->gaugeWriteDataClass(0x4218, &EOSConfiguration, sizeof(EOSConfiguration)))
          return false;

      /*
       * The user can set thresholds to alert the host when AccumulatedCharge() reaches a particular level in both
       * the charge (positive) and discharge (negative) directions. These thresholds are set by Accumulated
       * Charge Positive Threshold and Accumulated Charge Negative Threshold.
       */
      uint8_t accumChargeThrldPositive[2];
      //@TODO multiply by 0.85 to have some offset from ideal values
      accumChargeThrldPositive[0] = m_sysData.Capacity >> 8; //High byte first
      accumChargeThrldPositive[1] = m_sysData.Capacity;
      uint8_t accumChargeThrldNegative[2];
      accumChargeThrldNegative[0] = m_sysData.Capacity >> 8; //High byte first
      accumChargeThrldNegative[1] = m_sysData.Capacity;
      if(!this->gaugeWriteDataClass(0x416C, accumChargeThrldPositive, sizeof(accumChargeThrldPositive)))
        return false;
      if(!this->gaugeWriteDataClass(0x416E, accumChargeThrldNegative, sizeof(accumChargeThrldNegative)))
        return false;

      //////
      ///
      ///
      /// TO DO, Safety parameters update (BATHIGH, BATLOW, ... )
      ///

      if (!this->operationConfigA()) {
        return false;
      }
      if(!this->gaugeWriteDataClass(0x4155, &m_sysData.CellNumber, sizeof(m_sysData.CellNumber)))
        return false;

      uint8_t flashUpdateOkVoltage[2]; //500 mV
      flashUpdateOkVoltage[0] = 500 >> 8;
      if(!this->gaugeWriteDataClass(0x4157, flashUpdateOkVoltage, sizeof(flashUpdateOkVoltage)))
        return false;

      uint8_t dischargeDetectTreshld[2]; //2000 mA NORM? this is different from Learning disch current
      dischargeDetectTreshld[0] = 2000 >> 8;
      dischargeDetectTreshld[1] = 0xFF & 2000;
      uint8_t chargeDetectTreshld[2];    //2000 mA NORM?
      chargeDetectTreshld[0] = (200 / m_sysData.capScale) >> 8;
      chargeDetectTreshld[1] = 200 / m_sysData.capScale;

      if(!this->gaugeWriteDataClass(0x4162, dischargeDetectTreshld, sizeof(dischargeDetectTreshld)))
        return false;
      if(!this->gaugeWriteDataClass(0x4164, chargeDetectTreshld, sizeof(chargeDetectTreshld)))
        return false;
      uint8_t quitCurrent[2]; //Quit Current, mA
      quitCurrent[0] = 200 >> 8;
      quitCurrent[1] = 200;
      if(!this->gaugeWriteDataClass(0x4166, quitCurrent, sizeof(quitCurrent)))
        return false;

      //Manufacturing Status Init
      uint8_t data[2];
      data[0] = 0x00; //High byte first
      data[1] = 0x0B; //ACDSG_EN=1, ACCHG_EN=1, IGNORE_SD_EN, EOS_EN=1, PCTL_DIS, LF_DIS, WHR_DIS
      if(!this->gaugeWriteDataClass(0x40D7, data, sizeof(data)))
        return false;
      if (!CEDVConfig()) {
        return false;
      }
      updBatCondData();
      //@TODO init gas gauge config (when it understand voltage lvl) and safety levels

      return true;
    }

    bool bq34::chargeInit() {
      uint8_t data[2];
      data[0] = 300 >> 8; // Charge Current T1-T2 (300 mA)
      data[1] = 0xFF & 300;
      if(!this->gaugeWriteDataClass(0x410E, data, sizeof(data)))
        return false;
      data[0] = 300 >> 8; // Charge Current T2-T3 (300 mA)
      data[1] = 0xFF & 300;
      if(!this->gaugeWriteDataClass(0x4110, data, sizeof(data)))
        return false;
      data[0] = 300 >> 8; // Charge Current T3-T4 (300 mA)
      data[1] = 0xFF & 300;
      if(!this->gaugeWriteDataClass(0x4112, data, sizeof(data)))
        return false;
      data[0] = 2290 >> 8; // Charge Voltage T1-T2 (2290 * 12 -> 27480 mV)
      data[1] = 0xFF & 2290;
      if(!this->gaugeWriteDataClass(0x4114, data, sizeof(data)))
        return false;
      data[0] = 2290 >> 8; // Charge Voltage T2-T3 (2290 mV)
      data[1] = 0xFF & 2290;
      if(!this->gaugeWriteDataClass(0x4116, data, sizeof(data)))
        return false;
      data[0] = 2290 >> 8; // Charge Voltage T3-T4 (2290 mV)
      data[1] = 0xFF & 2290;
      if(!this->gaugeWriteDataClass(0x4118, data, sizeof(data)))
        return false;
      data[0] = 100 >> 8; //Maintenance Current (mA)
      data[1] = 100;
      if(!this->gaugeWriteDataClass(0x411A, data, sizeof(data)))
        return false;
      data[0] = 200 >> 8; //Taper Current (mA)
      data[1] = 200;
      if(!this->gaugeWriteDataClass(0x411C, data, sizeof(data)))
        return false;
      data[0] = 60 >> 8; //Minimum Taper Capacity (1 unit for 0.01 mAh)
      data[1] = 60;
      if(!this->gaugeWriteDataClass(0x411E, data, sizeof(data)))
        return false;
      data[0] = 100 >> 8; //Taper Voltage (mV)
      data[1] = 100;
      if(!this->gaugeWriteDataClass(0x4120, data, sizeof(data)))
        return false;
      data[0] = 60 >> 8; //Current Taper Window (s)
      data[1] = 60;
      if(!this->gaugeWriteDataClass(0x4122, data, sizeof(data)))
        return false;
      data[0] = 2290 >> 8; //Last Charge Voltage T1-T2 (mV)
      data[1] = 0xFF & 2290;
      if(!this->gaugeWriteDataClass(0x40C7, data, sizeof(data)))
        return false;
      data[0] = 2290 >> 8; //Last Charge Voltage T2-T3 (mV)
      data[1] = 0xFF & 2290;
      if(!this->gaugeWriteDataClass(0x40C9, data, sizeof(data)))
        return false;
      data[0] = 2290 >> 8; //Last Charge Voltage T3-T4 (mV)
      data[1] = 0xFF & 2290;
      if(!this->gaugeWriteDataClass(0x40CB, data, sizeof(data)))
        return false;
      return true;
    }

    void bq34::updEOSLearnStatus() {
      uint16_t tmp = 0;
      if(!this->getStdCommandData(bq34110::cmnd::EOSLERNSTAT, tmp))
        return;
      std::bitset<16> bset(tmp);
      bool *pTestLast = &m_EOSLernStatus.lcto;
      bool *pTestFirst = &m_EOSLernStatus.ldpai;
      bool *pStruct = (bool*)&m_EOSLernStatus;
      for(uint32_t i = sizeof(m_EOSLernStatus)-1; i>=0; i--) {
        if (bset.test(i)) {
          *pStruct = true;
        } else {
          *pStruct = false;
        }
      ++pStruct;
      }
/*
      if (bset.test(0)) {
        m_EOSLernStatus.ldsg = true;
      } else
        m_EOSLernStatus.ldsg = false;
      if (bset.test(1)) {
        m_EOSLernStatus.lchg = true;
      } else
        m_EOSLernStatus.lchg = false;
      if (bset.test(2)) {
        m_EOSLernStatus.lrlx = true;
      } else
        m_EOSLernStatus.lrlx = false;
      if (bset.test(3)) {
        m_EOSLernStatus.lper = true;
      } else
        m_EOSLernStatus.lper = false;
      if (bset.test(4)) {
        m_EOSLernStatus.lcmd = true;
      } else
        m_EOSLernStatus.lcmd = false;
      if (bset.test(5)) {
        m_EOSLernStatus.labrt = true;
      } else
        m_EOSLernStatus.labrt = false;
      if (bset.test(6)) {
        m_EOSLernStatus.lfault = true;
      } else
        m_EOSLernStatus.lfault = false;
      if (bset.test(7)) {
        m_EOSLernStatus.lcto = true;
      } else
        m_EOSLernStatus.lcto = false;
      if (bset.test(8)) {
        m_EOSLernStatus.ldpai = true;
      } else
        m_EOSLernStatus.ldpai = false;
      if (bset.test(9)) {
        m_EOSLernStatus.ldpat = true;
      } else
        m_EOSLernStatus.ldpat = false;
      if (bset.test(10)) {
        m_EOSLernStatus.ldpam = true;
      } else
        m_EOSLernStatus.ldpam = false;
      if (bset.test(11)) {
        m_EOSLernStatus.lucd = true;
      } else
        m_EOSLernStatus.lucd = false;
      if (bset.test(12)) {
        m_EOSLernStatus.lctledge = true;
      } else
        m_EOSLernStatus.lctledge = false;
      if (bset.test(13)) {
        m_EOSLernStatus.lrstor = true;
      } else
        m_EOSLernStatus.lrstor = false;
      if (bset.test(14)) {
        m_EOSLernStatus.lres = true;
      } else
        m_EOSLernStatus.lres = false;
      if (bset.test(15)) {
        m_EOSLernStatus.ldone = true;
      } else
        m_EOSLernStatus.ldone = false;
*/
      return;
    }

    void bq34::updBatStatus() {
      uint16_t tmp = 0;
      if(!this->getStdCommandData(bq34110::cmnd::BSTAT, tmp))
        return;
      std::bitset<16> bset(tmp);
      if (bset.test(0)) {
        m_batStatus.dsgIDetect = true;
      } else
        m_batStatus.dsgIDetect = false;
      if (bset.test(1)) {
        m_batStatus.chgIDetect = true;
      } else
        m_batStatus.chgIDetect = false;
      if (bset.test(2)) {
        m_batStatus.dsgAlarmTerminate = true;
      } else
        m_batStatus.dsgAlarmTerminate = false;
      if (bset.test(3)) {
        m_batStatus.chgAlarmTerminate = true;
      } else
        m_batStatus.chgAlarmTerminate = false;
      if (bset.test(4)) {
        m_batStatus.fullCharge = true;
      } else
        m_batStatus.fullCharge = false;
      if (bset.test(5)) {
        m_batStatus.fullDischarge = true;
      } else
        m_batStatus.fullDischarge = false;
      if (bset.test(6)) {
        m_batStatus.chgInhibit = true;
      } else
        m_batStatus.chgInhibit = false;
      if (bset.test(7)) {
        m_batStatus.sleep = true;
      } else
        m_batStatus.sleep = false;
      if (bset.test(8)) {
        m_batStatus.batLow = true;
      } else
        m_batStatus.batLow = false;
      if (bset.test(9)) {
        m_batStatus.batHigh = true;
      } else
        m_batStatus.batHigh = false;
      if (bset.test(10)) {
        m_batStatus.overTmpInDsgMode = true;
      } else
        m_batStatus.overTmpInDsgMode = false;
      if (bset.test(11)) {
        m_batStatus.overTmpInChgMode = true;
      } else
        m_batStatus.overTmpInChgMode = false;
      if (bset.test(12)) {
        m_batStatus.lowTmpInDsgMode = true;
      } else
        m_batStatus.lowTmpInDsgMode = false;
      if (bset.test(13)) {
        m_batStatus.lowTmpInChgMode = true;
      } else
        m_batStatus.lowTmpInChgMode = false;
      if (bset.test(14)) {
        m_batStatus.SOCLow = true;
      } else
        m_batStatus.SOCLow = false;
      return;
    }
    void bq34::updBatCondData() {
      uint16_t tmp;
      if (!getStdCommandData(bq34110::cmnd::VOLT, tmp)) {
        m_batCond.voltage = 0xFFFF;
      }
      m_batCond.voltage = tmp;
      if (!getStdCommandData(bq34110::cmnd::Current, tmp)) {
        m_batCond.current = 0xFFFF;
      }
      m_batCond.current = tmp * m_sysData.capScale;
      if (!getStdCommandData(bq34110::cmnd::RC, tmp)) {
        m_batCond.remCap = 0xFFFF;
      }
      m_batCond.remCap = tmp * m_sysData.capScale;
      if (!getStdCommandData(bq34110::cmnd::FCC, tmp)) {
        m_batCond.fullChgCap = 0xFFFF;
      }
      m_batCond.fullChgCap = tmp * m_sysData.capScale;
      if (!getStdCommandData(bq34110::cmnd::ACCCHAR, tmp)) {
        m_batCond.acummCharge = 0xFFFF;
      }
      m_batCond.acummCharge = tmp * m_sysData.capScale;
    }

  bool bq34::isVoltNorm() {
    uint16_t voltage_mV = 0;
    if (getStdCommandData(bq34110::cmnd::VOLT, voltage_mV)) {
      if (voltage_mV < 27000) {
        return false;
      }
      return true;
    }
    return false;
  }

  uint32_t bq34::getVoltage() {
    uint16_t voltage_mV = 0;
    if (getStdCommandData(bq34110::cmnd::VOLT, voltage_mV)) {
      return voltage_mV;
    }
    return 9999;
  }

  void bq34::startTest(){
    if(gaugeControlSubCmnd(bq34110::subcmnd::ACCUM_RESET)) {
      updBatCondData();
//    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET); //DRAINING LOAD ON
      if (!gaugeControlSubCmnd(bq34110::subcmnd::EOS_LOAD_ON)) { //for test at home ONLY
        m_batStatus.testStarded = false;
        return;
      }
      m_batStatus.testStarded = true;
      HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET); //OUTPUT PIN HIGH, TEST IS GOING  //for tests LED selected
    }
  }

  void bq34::checkTestCondition(uint32_t& cntr){
    if (m_batStatus.testStarded && (m_batCond.acummCharge * 100 / m_sysData.Capacity) > m_sysData.lowCapAlert_prct) {
//      HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET); // DRAINING LOAD OFF
gaugeControlSubCmnd(bq34110::subcmnd::EOS_LOAD_OFF);// for test at home ONLY
//      HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET); // OUTPUT PIN LOW, TEST IS NOT-GOING
      cntr++;
      m_batStatus.testStarded = false;
    } else if (m_batStatus.SOCLow && m_batStatus.testStarded) {
      //HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_RESET); //DRAINING LOAD OFF
//      m_batStatus.testStarded = false;
      if (gaugeControlSubCmnd(bq34110::subcmnd::EOS_LOAD_OFF)) { //for test at home ONLY
        m_batStatus.testStarded = false;
        //HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_SET); //ALERT OF LOW CAPACITY
      }
      HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET); //OUTPUT PIN LOW, TEST IS NOT-GOING
      if(m_batCond.acummCharge * 100 / m_sysData.Capacity < m_sysData.lowCapAlert_prct) {    //in DSG accumCharge is growing (opposite in CHG)
        //HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_SET); //ALERT OF LOW CAPACITY
      }
    }
  }

  bool bq34::isTestStarted(){
    return m_batStatus.testStarded;
  }

}
