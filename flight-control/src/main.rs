use esp_idf_sys as _; // If using the `binstart` feature of `esp-idf-sys`, always keep this module imported

use esp_idf_hal::i2c::*;
use esp_idf_hal::peripherals::Peripherals;
use esp_idf_hal::prelude::*;
use std::thread;
use std::sync::{Arc, Mutex};

mod lcd;
mod joystick;
mod lora;
mod throttle;
mod common;


fn main() {
    // It is necessary to call this function once. Otherwise some patches to the runtime
    // implemented by esp-idf-sys might not link properly. See https://github.com/esp-rs/esp-idf-template/issues/71
    esp_idf_sys::link_patches();
    println!("csokiii");

    let peripherals = Peripherals::take().unwrap();
    let i2c = peripherals.i2c0;
    let scl = peripherals.pins.gpio22;
    let sda = peripherals.pins.gpio21;
    



    let config = I2cConfig::new().baudrate(100.kHz().into());
    let mut i2c_driver = I2cDriver::new(i2c, sda, scl, &config).unwrap();

    let mut lcd_2004A = lcd::LCD::new();

    lcd_2004A.init_lcd(&mut i2c_driver);
    lcd_2004A.draw_data(&mut i2c_driver, common::ControlData::new()); 
    

}
