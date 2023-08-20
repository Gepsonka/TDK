use core::time;
use std::sync::{Arc, Mutex};
use std::thread;
use aes_gcm::aead::consts::U12;
use aes_gcm::{Aes128Gcm, AesGcm, KeyInit, Aes256Gcm};
use aes_gcm::aead::OsRng;
use aes_gcm::aes::Aes128;
use env_logger::Target;
use log::{debug, info};
use rppal::gpio::{Gpio, InputPin, Trigger};
use rppal::spi::{Bus, Mode, SlaveSelect, Spi};
use env_logger;
use crate::network::arp_table::ArpTable;
use crate::network::lora;
use crate::network::lora::Bandwidth;
use crate::network::packet::LoRaPacket;
use crate::network::queue::Queue;
pub mod network;


fn receive_callback(packet: Vec<u8>) {
    println!("Receive callback");
}

fn main() {
    let mut builder = env_logger::Builder::from_default_env();
    builder.target(Target::Stdout);
    env_logger::init();


    let mut int_pin:InputPin = Gpio::new().unwrap().get(5).unwrap().into_input_pulldown();
    let mut reset_pin = Gpio::new().unwrap().get(6).unwrap().into_output();
    reset_pin.set_high();
    int_pin.set_interrupt(Trigger::RisingEdge).unwrap();


    let mut lora_device = lora::LoRa::new(
        Spi::new(Bus::Spi0, SlaveSelect::Ss0, 10_000_000, Mode::Mode0).unwrap(),
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

    lora_device.set_opmod(lora::OperationMode::SX127x_MODE_SLEEP).unwrap();
    lora_device.set_frequency(437200012).unwrap();
    lora_device.reset_fifo().unwrap();
    lora_device.set_opmod(lora::OperationMode::SX127x_MODE_STANDBY).unwrap();
    lora_device.set_bandwidth(Bandwidth::SX127x_BW_125000).unwrap();
    lora_device.set_header_type(lora::HeaderMode::Explicit).unwrap();
    lora_device.set_modem_config_2(lora::SpreadFactor::SX127x_SF_9).unwrap();
    lora_device.set_syncword(0x12).unwrap();
    lora_device.set_preamble_length(8).unwrap();
    lora_device.set_bandwidth(Bandwidth::SX127x_BW_250000).unwrap();
    lora_device.set_opmod(lora::OperationMode::SX127x_MODE_RX_CONT).unwrap();

    info!("LoRa init done!");


    let arp_table_arc: Arc<Mutex<ArpTable<u8, LoRaPacket, AesGcm<Aes128, U12>, U12>>> = Arc::new(Mutex::new(network::arp_table::ArpTable::new()));
    let lora_arc = Arc::new(Mutex::new(lora_device));
    let tx_queue_arc = Arc::new(Mutex::new(network::transmit_queue::TransmitQueue::<LoRaPacket>::new()));
    let rx_queue_arc = Arc::new(Mutex::new(network::receive_queue::ReceiveQueue::new()));
    let black_list_arc = Arc::new(Mutex::new(network::blacklist::BlackList::new()));

    network::transmit_queue::TransmitQueue::packet_tx_thread(
        &tx_queue_arc,
        &lora_arc,
    );

    network::receive_queue::ReceiveQueue::packet_dispatch_thread(
        &rx_queue_arc,
        &arp_table_arc,
        &black_list_arc,
    );

    thread::spawn(move || {
        let mut packet_to_tx: LoRaPacket;
        let mut tx_queue_mutex = Arc::clone(&tx_queue_arc);
        loop {
            thread::sleep(time::Duration::from_secs(1));
            packet_to_tx = LoRaPacket::try_from(vec![0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B]).unwrap();
            let mut tx_queue = tx_queue_mutex.lock().unwrap();
            tx_queue.push(packet_to_tx.clone());
            tx_queue.push(packet_to_tx.clone());
            tx_queue.push(packet_to_tx.clone());
            debug!("sending packet");
        }
    });

    let mut lora = Arc::clone(&lora_arc);

    loop {
        int_pin.poll_interrupt(true, None).unwrap();
        let mut lora_thr = lora.lock().unwrap();
        lora_thr.handle_interrupt().unwrap();
        debug!("tx status: {:?}", lora_thr.waiting_for_tx);

    }

    let key = Aes256Gcm::generate_key(OsRng);
    let cipher = Aes256Gcm::new(&key);

}
