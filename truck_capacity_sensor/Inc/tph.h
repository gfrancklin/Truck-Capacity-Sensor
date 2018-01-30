/*
 * tph.h
 *
 *  Created on: Aug 16, 2017
 *      Author: jchapple4119
 */

#ifndef TPH_H_
#define TPH_H_

#define	ADC_TIMEOUT			2		// ADC timeout in milliseconds
#define	ADC_READINGS		10000	// number of ADC readings to take for one valid reading

// For MS8607 series temperature, pressure, humidity sensor on I2C bus
// TP sensor defines
#define	RAW_I2C_BYTES		4		// maximum number of i2c data bytes

#define	TP_ADDRESS			0xec	// address is shifted up one bit per HAL docs

#define	TP_RESET_CMD		0x1e
#define	TP_PROM_READ_CMD	0xa0	// prom address is in bits 3 downto 1
#define	TP_PROM_WORDS		6
#define	ADC_READ			0x0
#define	PRESSURE_READ		0x4a	// D1
#define	TEMPERATURE_READ	0x5a	// D2
#define	TP_READ_TIME		20		// temperature and pressure adc conversion times plus margin
// time in milliseconds

// Humidity sensor defines
#define	H_ADDRESS			0x80	// address is shifted up one bit per HAL docs

#define	H_RESET				0xfe
#define	H_PROM_READ			0xa0	// prom address is in bits 3 downto 1

// For barometric pressure sensor designed by John Chapple

#define	HI_COUNT			1748.7	// high pressure ADC count
#define	LO_COUNT			2379.7	// low pressure ADC count

#define	HI_PRESSURE			304		// high pressure difference in millimeters of water
#define	LO_PRESSURE			-260	// low pressure difference in millimeters of water

#define	COUNT_DIFFERENCE	(HI_COUNT - LO_COUNT)
#define	PRESSURE_DIFFERENCE	(HI_PRESSURE - LO_PRESSURE)

#define	MM_PASCAL_FACTOR	9.80665	// units: Pa/mmH20

#define	BPS_SLOPE			(((float) PRESSURE_DIFFERENCE) / ((float) COUNT_DIFFERENCE) * MM_PASCAL_FACTOR)

#define	BPS_CONSTANT		120.1E3	// Constant to be added to allow for increased gain in electronics
									// to maximise precision of sensor; minimum reading 90kPa,
									// maximum reading 110kPa


#endif /* TPH_H_ */
