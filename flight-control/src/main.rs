use common::ControlData;
use esp_idf_hal::delay::{FreeRtos, BLOCK};
use esp_idf_hal::uart::UartDriver;
use esp_idf_sys as _; // If using the `binstart` feature of `esp-idf-sys`, always keep this module imported

use esp_idf_hal::{i2c::*, uart, gpio};
use esp_idf_hal::peripherals::Peripherals;
use esp_idf_hal::prelude::*;
use std::thread;
use std::sync::{Arc, Mutex};
use std::time::Duration;
use esp_idf_hal::adc::{self, Atten11dB, AdcChannelDriver};
use esp_idf_hal::adc::config::Config;

use esp_idf_hal::gpio::{Gpio27, PinDriver, Pull};

mod lcd;
mod joystick;
mod lora;
mod throttle;
mod common;
mod altimeter;
mod concurrency;

// TODO: Clean up unwraps everywhere, change function return types to results.S

fn main() {
    // It is necessary to call this function once. Otherwise some patches to the runtime
    // implemented by esp-idf-sys might not link properly. See https://github.com/esp-rs/esp-idf-template/issues/71
    esp_idf_sys::link_patches();

    let peripherals = Peripherals::take().unwrap();
    let i2c = peripherals.i2c0;
    let scl = peripherals.pins.gpio22;
    let sda = peripherals.pins.gpio21;

    

    let mut n_switch = PinDriver::input(peripherals.pins.gpio32).unwrap();
    let mut s_switch = PinDriver::input(peripherals.pins.gpio33).unwrap();
    let mut e_switch = PinDriver::input(peripherals.pins.gpio25).unwrap();
    let mut w_switch = PinDriver::input(peripherals.pins.gpio26).unwrap();

    n_switch.set_pull(Pull::Down).unwrap();
    e_switch.set_pull(Pull::Down).unwrap();
    w_switch.set_pull(Pull::Down).unwrap();
    s_switch.set_pull(Pull::Down).unwrap();

    let config = I2cConfig::new().baudrate(100.kHz().into());
    let mut i2c_driver = I2cDriver::new(i2c, sda, scl, &config).unwrap();

    let uart_tx = peripherals.pins.gpio17;
    let uart_rx = peripherals.pins.gpio16;

    let uart_config = uart::config::Config::new().baudrate(Hertz(9600));
    let uart = UartDriver::new(
        peripherals.uart2,
        uart_tx,
        uart_rx,
        Option::<gpio::Gpio0>::None,
        Option::<gpio::Gpio1>::None,
        &uart_config,
    ).unwrap();

    // Needed to optimize the output drawing to the LCD
    let mut control_data = Arc::new(Mutex::new(ControlData::new(
        adc::AdcDriver::new(peripherals.adc2, &Config::new().calibration(true)).unwrap(),
        AdcChannelDriver::new(peripherals.pins.gpio27).unwrap(),
        n_switch,
        w_switch,
        e_switch,
        s_switch

    )));

    let data_control_thread = thread::spawn(move || {
        //let control_data_ref = control_data.clone();
        loop{
            ControlData::update_data(control_data.clone());
            thread::sleep(Duration::from_millis(1000));
        }
        
    });


    let mut lcd_2004A = lcd::LCD::new();
    lcd_2004A.init_lcd(&mut i2c_driver).unwrap();
    //lcd_2004A.draw_data(&mut i2c_driver, common::ControlData::new()).unwrap();

    //data_control_thread.join().unwrap();
    
    let mut buf = [0u8; 1024];
    let n = uart.read(&mut buf, BLOCK).unwrap();
    let data = String::from_utf8(buf[..n].to_vec()).unwrap();
    println!("Received data:\n{}", data);
    

    loop {
        //println!("Csokiiii");
        FreeRtos::delay_ms(1000);
    }

    //data_control_thread.join().unwrap();

}
