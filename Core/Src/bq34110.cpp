#include "bq34110.h"
#include "stdint.h"
#include "i2c.h"
#include "stdio.h"

namespace  {
    uint8_t capInitPerScale = 50; //this scale % DOD voltage in 2 times
    int32_t *voltageDOD = new int32_t[10];
    void setConfigForInit()
    {
      //start flashing led
      //if no button is pressed during 5 sec, then config 1, if pressed 1 time - conf 2 and so on...
      capInitPerScale = 50;

      voltageDOD[0] = 22500; //45000mV (22.5 * capInitPerScale)
      voltageDOD[1] = 20250;
      voltageDOD[2] = 18000;
      voltageDOD[3] = 15750;
      voltageDOD[4] = 13500;
      voltageDOD[5] = 11250;
      voltageDOD[6] = 9000;
      voltageDOD[7] = 6750;
      voltageDOD[8] = 22500;
      voltageDOD[9] = 22500;

    }
}

namespace bq34110 {

    bool bq34::init()
    {
      setConfigForInit();
      if (!this->operationConfigA()) {
        return false;
      }
      //Manufacturing Status Init
      uint8_t data[2];
      data[0] = 0x00; //High byte first
      data[1] = 0x0B; //ACDSG_EN, ACCHG_EN, IGNORE_SD_EN, EOS_EN, PCTL_DIS, LF_DIS, WHR_DIS
      if(!this->gaugeWriteDataClass(0x40D7, data, sizeof(data)))
        return false;

    }

    bool bq34::getStdCommandData(uint8_t cmnCode, uint16_t& data)
    {
        uint8_t data_tmp[2];
        if(!(this->gaugeRead(cmnCode, data_tmp, sizeof(data_tmp))))
        {
        	return false;
        } else {
          data = data_tmp[1] | (data_tmp[0] << 8);
          return true;
        }
    }

    bool bq34::getSubCommandData(uint8_t cmnCode, uint16_t subCmnd, uint16_t& data)
    {
        uint8_t i2c_data[3];
        i2c_data[0] = cmnCode;
        i2c_data[1] = subCmnd;
        i2c_data[2] = 0xFF & subCmnd >> 8;
        if(HAL_I2C_Master_Transmit(&hi2c1, static_cast<uint16_t>(bq34110::selfAddress), i2c_data, sizeof(i2c_data), 100) != HAL_OK)
          return false;
        HAL_Delay(1500);
        return this->getStdCommandData(cmnCode, data);
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
    bool bq34::gaugeWriteDataClass(uint16_t subClass, uint8_t *pData, uint8_t dataLen)
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

    bool bq34::operationConfigA()
    {
      uint16_t configAdr = 0x413A;
      uint8_t i2c_data[5];
      i2c_data[0] = bq34110::cmnd::MAC;
      i2c_data[1] = configAdr;
      i2c_data[2] = 0xFF & configAdr >> 8;
      /*
       * Write data is BigEndian format
       */
      i2c_data[3] = 1;    //confgih High byte
      i2c_data[4] = 2 ;   //config Low byte
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
      uint8_t i2c_data[5];
      i2c_data[0] = bq34110::cmnd::MAC;
      i2c_data[1] = configAdr;
      i2c_data[2] = 0xFF & configAdr >> 8;
      /*
       * Write data is BigEndian format
       */
      i2c_data[3] = 1;    //confgih High byte
      i2c_data[4] = 2 ;   //config Low byte
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

    bool bq34::gaugeRead(uint8_t cmnd, uint8_t *pData, uint8_t dataLen)
    {
      uint8_t cmndDat[] = {cmnd};
      if (HAL_I2C_Master_Transmit(&hi2c1, static_cast<uint16_t>(bq34110::selfAddress), cmndDat, sizeof(cmnd), 10) != HAL_OK) {
        return false;
      }
      if (HAL_I2C_Master_Receive(&hi2c1, static_cast<uint16_t>(bq34110::selfAddress), pData, dataLen, 1500) != HAL_OK) {
        return false;
      } else
        return true;
    }

    bool bq34::gaugeWrite(uint8_t *pData, uint8_t dataLen)
    {
      if (HAL_I2C_Master_Transmit(&hi2c1, static_cast<uint16_t>(bq34110::selfAddress), pData, dataLen, 10) != HAL_OK)
      {
        return false;
      } else {
        return true;
      }
    }

}
