use std::collections::VecDeque;
use std::sync::{Arc, Mutex};
use std::thread;
use log::{debug, error, info};
use crate::network::lora::OperationMode;
use crate::network::queue::Queue;
use super::{packet::LoRaPacket, lora::{LoRa, self}};



pub struct TransmitQueue<PacketT>
where PacketT: Into<PacketT> + TryFrom<PacketT> + From<PacketT> + Clone
{
    queue: VecDeque<PacketT>
}


impl TransmitQueue<LoRaPacket> {
    pub fn new() -> Self {
        TransmitQueue {
            queue: VecDeque::new()
        }
    }

    pub fn packet_tx_thread(tx_queue_rc: &Arc<Mutex<Self>>, lora_device_rc: &Arc<Mutex<LoRa>>) {
        let mut tx_queue_mutex = Arc::clone(tx_queue_rc);
        let mut lora_device_mutex = Arc::clone(lora_device_rc);
        info!("Starting packet transmit thread");
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
                    if !lora_device.waiting_for_tx {
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
            }
            debug!("Packet transmit thread exiting");
        });
    }
}

impl <PacketT> Queue<PacketT> for TransmitQueue<PacketT>
    where PacketT: Into<PacketT> + TryFrom<PacketT> + From<PacketT> + Clone,
{
    fn get_top_item(&mut self) -> Option<PacketT> {
        self.queue.front().cloned()
    }

    fn pop(&mut self) -> Option<PacketT> {
        self.queue.pop_front()
    }

    fn push(&mut self, item: PacketT) {
        self.queue.push_back(item);
    }

    fn is_empty(&self) -> bool {
        self.queue.is_empty()
    }

    fn len(&self) -> usize {
        self.queue.len()
    }
}