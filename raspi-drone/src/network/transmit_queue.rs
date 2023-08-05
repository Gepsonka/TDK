use std::collections::VecDeque;
use std::sync::{Arc, Mutex};
use std::thread;
use log::error;
use crate::network::lora::OperationMode;
use super::{packet::LoRaPacket, lora::{LoRa, self}};



pub struct TransmitQueue<PacketT>
where PacketT: Into<LoRaPacket> + From<LoRaPacket> + Clone
{
    queue: VecDeque<PacketT>
}


impl TransmitQueue<LoRaPacket> {
    pub fn new() -> Self {
        TransmitQueue {
            queue: VecDeque::new()
        }
    }

    pub fn packet_tx_thread(tx_queue_rc: Arc<Mutex<Self>>, lora_device_rc: Arc<Mutex<LoRa>>) {
        let mut tx_queue_mutex = Arc::clone(&tx_queue_rc);
        let mut lora_device_mutex = Arc::clone(&lora_device_rc);
        let packet_tx_thread_handle = thread::spawn(move || {
            loop {
                if !tx_queue_mutex.lock().unwrap().queue.is_empty() {
                    loop {
                        if !lora_device_mutex.lock().unwrap().waiting_for_tx {
                            break;
                        }
                    }
                    let mut lora_device = lora_device_mutex.lock().unwrap();
                    let packet = tx_queue_mutex.lock().unwrap().queue.pop_front().unwrap();
                    let lora_packet_raw: Vec<u8> = packet.into();
                    lora_device.set_for_transmission(lora_packet_raw.as_slice()).unwrap();
                    lora_device.set_opmod(OperationMode::SX127x_MODE_TX).unwrap();
                } else {
                    let mut lora_device = lora_device_mutex.lock().unwrap();
                    if let Some(opmode) = lora_device.opmod {
                        match opmode {
                            OperationMode::SX127x_MODE_RX_CONT => {},
                            _ => lora_device.set_opmod(OperationMode::SX127x_MODE_RX_CONT).expect("Cannot set opmode to RX_CONT")
                        }
                    } else {
                        error!("LoRa opmode is not set.");
                        panic!("LoRa opmode is not set.");
                    }
                }
            }
        });
    }
}