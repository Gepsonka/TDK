use common::ControlData;
use esp_idf_hal::delay::{FreeRtos, BLOCK};
use esp_idf_hal::uart::UartDriver;
use esp_idf_sys as _; // If using the `binstart` feature of `esp-idf-sys`, always keep this module imported
use esp_idf_hal::{i2c::*, uart, gpio};
use esp_idf_hal::peripherals::Peripherals;
use esp_idf_hal::prelude::*;
use gps::GPS;
use lora::{LoRa, ModemConfig, CodingRate, SpreadingFactor};
use std::thread;
use std::sync::{Arc, Mutex};
use std::time::Duration;
use esp_idf_hal::adc::{self, AdcChannelDriver};
use esp_idf_hal::adc::config::Config;
use esp_idf_hal::gpio::{ PinDriver, Pull};
use esp_idf_hal;
use esp_idf_hal::spi::{SPI1, SPI2};
use esp_idf_hal::interrupt;
use embedded_hal::spi::Mode;
use embedded_hal::spi::MODE_0;


mod lcd;
mod joystick;
mod lora;
mod throttle;
mod common;
mod concurrency;
mod gps;

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


    let spi = peripherals.spi2;
    let rst = PinDriver::output(peripherals.pins.gpio0).unwrap();
    let mut int_pin = PinDriver::input(peripherals.pins.gpio4).unwrap();
    int_pin.set_pull(Pull::Down).unwrap();
    let mut dio1 = PinDriver::input(peripherals.pins.gpio2).unwrap();
    dio1.set_pull(Pull::Down).unwrap();
    let sclk = peripherals.pins.gpio18;
    let mosi = peripherals.pins.gpio19;
    let miso = peripherals.pins.gpio23;
    let cs = peripherals.pins.gpio5;

    let config = esp_idf_hal::spi::config::Config::new()
        .baudrate(4.MHz().into())
        .data_mode(MODE_0);


    let spi_driver = esp_idf_hal::spi::SpiDeviceDriver::new_single(spi, sclk, mosi, Some(miso), esp_idf_hal::spi::Dma::Disabled, Some(cs), &config).unwrap();


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
            FreeRtos::delay_ms(100);
            ControlData::update_controls_thread(control_data_control_thread_clone.clone());
        }
    });


    let mut lcd_2004A = lcd::LCD::new();
    lcd_2004A.init_lcd(&mut i2c_driver.lock().unwrap()).unwrap();
    lcd_2004A.draw_flight_data_template(&mut i2c_driver.lock().unwrap());


    let mut modem_config = ModemConfig {
        bandwidth: lora::Bandwidth::K125,
        coding_rate: CodingRate::K12,
        spreading_factor: SpreadingFactor::SF6,
        implicit_header: true,
        rx_continuous: false,
        crc_on: true,
    };

    let mut lora_transceiver = LoRa::new(spi_driver, rst, int_pin, dio1, 433);
    lora_transceiver.init(modem_config).unwrap();
    lora_transceiver.send(&[23, 12, 43]).unwrap();
    
    
    //lcd_2004A.draw_data(&mut i2c_driver, common::ControlData::new()).unwrap();

    let mut control_data_lcd_thread_clone = control_data.clone();
    let mut i2c_lcd_thread_clone = i2c_driver.clone();

    let lcd_control_thread = thread::spawn(move || {
        loop{
            FreeRtos::delay_ms(100);
            lcd_2004A.lcd_thread(control_data_lcd_thread_clone.clone(), i2c_lcd_thread_clone.clone());
            
        }
        
    });

    
    // let mut buf = [0u8; 1024];
    // let n = uart.read(&mut buf, BLOCK).unwrap();
    // let data = String::from_utf8(buf[..n].to_vec()).unwrap();
    // println!("Received data:\n{}", data);
    
    let mut gps = GPS::new(uart);
    gps.init_gps().unwrap();

    loop {
        //let gps_data = gps.get_data().unwrap();
        // println!("{}", gps.get_raw_data().unwrap());
        FreeRtos::delay_ms(1000);
        lora_transceiver.send(&[12, 32, 12, 41, 54]).unwrap();
    }

    //data_control_thread.join().unwrap();

}
