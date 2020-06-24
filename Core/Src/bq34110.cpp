#include "bq34110.h"
#include "stdint.h"
#include "i2c.h"
#include "stdio.h"

namespace  {
	
}

namespace bq34110 {
    uint16_t bq34::getCommandData(uint8_t cmnCode)
    {
        uint16_t data = NULL;
        if(HAL_I2C_Master_Transmit(&hi2c1, bq34110::selfAddress, &cmnCode, sizeof(cmnCode), 10000) != HAL_OK)
        	return data;
        auto pData = reinterpret_cast <uint8_t*>(&data);
        if(HAL_I2C_Master_Receive(&hi2c1, bq34110::selfAddress, pData, sizeof(data), 10000) != HAL_OK)
        	return data;
        return data;
    }
}
