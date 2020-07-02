#include "bq34110.h"
#include "stdint.h"
#include "i2c.h"
#include "stdio.h"

namespace  {

}

namespace bq34110 {

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
      uint8_t data_tmp;
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

    }

    /*
     *  Write data for a gauge standard command, for exmaple: change design capacity and design energy.
     * subClass - Subclass
     * pData -  holds returned data from the gauge
     *
     * To change parameters, first identify the data class (also called subclass) from the TRM. In this example,
      the two parameters reside in the data class State at offset 10 (design capacity) and offset 12 (design
      energy). Read the whole data class into a byte buffer with a size equal to integer multiples of 32. The size
      must be greater or equal of the largest offset (plus the data type length) within the data class. This
      information is in the TRM. In this example, the largest offset in data class State is 39 with a data type
      length of two bytes (data type = I2) so the whole data class size is 2 × 32 = 64. Change the parameter in
      the byte buffer followed by writing the whole data class

      char pData[DC_STATE_LENGTH]; //DC_STATE_LENGTH = 64
      gauge_read_data_class(pI2C, DC_STATE, pData, DC_STATE_LENGTH);
      pData[10] = (DESIGN_CAPACITY & 0xFF00) >>8;
      pData[11] = DESIGN_CAPACITY & 0xFF;
      pData[12] = (DESIGN_ENERGY & 0xFF00) >>8;
      pData[13] = DESIGN_ENERGY & 0xFF;

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

    bool bq34::gaugeRead(uint8_t cmnd, uint8_t *pData, uint8_t dataLen)
    {
      uint8_t cmndDat[1] = {cmnd};
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
