use esp_idf_hal::{i2c::I2C0, gpio::{Gpio5, Gpio6}};


const LCD_DRIVER_I2C_ADDRESS: u8 = 0x4E;

pub struct LCD <'a> {
    i2c: &'a mut I2C0,
    sda: &'a mut Gpio5,
    scl: &'a mut Gpio6
}

impl LCD<'_> {
    pub fn new<'a>(i2c: &'a mut I2C0, sda: &'a mut Gpio5, scl: &'a mut Gpio6) -> LCD<'a> {
        return LCD { i2c: i2c, sda: sda, scl: scl };
    }

}