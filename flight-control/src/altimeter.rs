use esp_idf_hal::{i2c::I2cDriver, delay::BLOCK};


const ALTIMETER_I2C_ADDRESS: u8 = 0x76;

const BMP280_CTRL_MEAS_REG: u8 = 0xF4;

const BMP280_TEMP_REG: u8 = 0xFA; 
const BMP280_PRESS_REG:u8 = 0xF7;

pub struct Altimeter {
    temperature: i8,
    pressure: u16
}


impl Altimeter {
    pub fn new() -> Altimeter {
        Altimeter{temperature: 0, pressure: 0}
    }

    pub fn init(&mut self, i2c: &mut I2cDriver) -> Result<(), esp_idf_sys::EspError> {
        return i2c.write(ALTIMETER_I2C_ADDRESS, &[BMP280_CTRL_MEAS_REG, 0x2F], BLOCK);
    }

    pub fn read_temperature(&mut self, i2c: &mut I2cDriver){
        let mut data: [u8; 3] = [0, 0, 0];
        i2c.write_read(ALTIMETER_I2C_ADDRESS, &[BMP280_TEMP_REG], &mut data, BLOCK).unwrap();
        println!("{:?}", data);
        // (int32_t)(((int32_t)data[0] << 16) | ((int32_t)data[1] << 8) | (int32_t)data[2]) >> 4
        let converted_value: i32 = ((data[0] as i32) << 16 | (data[1] as i32) << 8 | (data[2] as i32)) >> 4;


        println!("temperature: {}", converted_value / 100);
    }
}
