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
use esp_idf_hal::adc::{self, AdcChannelDriver};
use esp_idf_hal::adc::config::Config;

use esp_idf_hal::gpio::{ PinDriver, Pull};

mod lcd;
mod joystick;
mod lora;
mod throttle;
mod common;
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
    let mut i2c_driver = Arc::new(Mutex::new(I2cDriver::new(i2c, sda, scl, &config).unwrap()));

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

    let control_data_control_thread_clone = control_data.clone();

    let data_control_thread = thread::spawn(move || {
        loop{
            ControlData::update_controls_thread(control_data_control_thread_clone.clone());
            FreeRtos::delay_ms(100);
        }
        
    });


    let mut lcd_2004A = lcd::LCD::new();
    lcd_2004A.init_lcd(&mut i2c_driver.lock().unwrap()).unwrap();
    lcd_2004A.draw_flight_data_template(&mut i2c_driver.lock().unwrap());
    
    
    //lcd_2004A.draw_data(&mut i2c_driver, common::ControlData::new()).unwrap();

    let mut control_data_lcd_thread_clone = control_data.clone();
    let mut i2c_lcd_thread_clone = i2c_driver.clone();

    let lcd_control_thread = thread::spawn(move || {
        loop{
            lcd_2004A.lcd_thread(control_data_lcd_thread_clone.clone(), i2c_lcd_thread_clone.clone());
            FreeRtos::delay_ms(100);
        }
        
    });

    
    // let mut buf = [0u8; 1024];
    // let n = uart.read(&mut buf, BLOCK).unwrap();
    // let data = String::from_utf8(buf[..n].to_vec()).unwrap();
    // println!("Received data:\n{}", data);
    

    loop {
        FreeRtos::delay_ms(1000);
    }

    //data_control_thread.join().unwrap();

}
