use core::time;
use std::thread;
use env_logger::Target;
use log::{debug, info};
use rppal::gpio::Gpio;
use rppal::spi::{Bus, Mode, SlaveSelect, Spi};
use env_logger;


pub mod network;

use crate::network::lora;
use crate::network::lora::Bandwidth;

fn receive_callback() {
    println!("Receive callback");
}

fn main() {
    let mut builder = env_logger::Builder::from_default_env();
    builder.target(Target::Stdout);
    env_logger::init();


    let mut int_pin = Gpio::new().unwrap().get(5).unwrap().into_input_pulldown();
    let mut reset_pin = Gpio::new().unwrap().get(6).unwrap().into_output();
    reset_pin.set_high();

    let mut lora_device = lora::LoRa::new(
        Spi::new(Bus::Spi0, SlaveSelect::Ss0, 10_000_000, Mode::Mode0).unwrap(),
        int_pin,
        reset_pin,
        lora::HeaderMode::Explicit,
        437200012,
        receive_callback,
        || {
            debug!("Transmit callback")
        }
    ).expect("Error lora init!");

    lora_device.reset();
    info!("LoRa version is correct: {:x}",lora_device.check_version().unwrap());

    lora_device.write_register(lora::REG_OP_MODE, &[0x00]).unwrap();
    lora_device.write_register(lora::REG_OP_MODE, &[0x80]).unwrap();
    lora_device.write_register(lora::REG_OP_MODE, &[0x80 | lora::OperationMode::SX127x_MODE_STANDBY as u8]).unwrap();
    lora_device.set_opmod(lora::OperationMode::SX127x_MODE_SLEEP).unwrap();
    lora_device.set_frequency(437200012).unwrap();
    lora_device.reset_fifo().unwrap();
    lora_device.set_opmod(lora::OperationMode::SX127x_MODE_STANDBY).unwrap();
    lora_device.set_bandwidth(Bandwidth::SX127x_BW_125000).unwrap();
    lora_device.set_header_type(lora::HeaderMode::Explicit).unwrap();
    lora_device.set_modem_config_2(lora::SpreadFactor::SX127x_SF_9).unwrap();
    lora_device.set_syncword(0x12).unwrap();
    lora_device.set_preamble_length(8).unwrap();

    thread::sleep(time::Duration::from_secs(1));

    lora_device.set_bandwidth(Bandwidth::SX127x_BW_10400).unwrap();

    thread::sleep(time::Duration::from_secs(1));

    lora_device.set_opmod(lora::OperationMode::SX127x_MODE_RX_CONT).unwrap();




    loop {
        thread::sleep(time::Duration::from_secs(1));
        //lora_device.set_bandwidth(Bandwidth::SX127x_BW_500000).unwrap();
    }
}
