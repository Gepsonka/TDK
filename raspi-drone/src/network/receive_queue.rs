use std::collections::VecDeque;
use std::hash::Hash;
use std::sync::{Arc, Mutex};
use std::thread;
use aes_gcm::{Aes256Gcm, AesGcm, Key, KeyInit, Nonce};
use aes_gcm::aead::consts::U12;
use aes_gcm::aead::OsRng;
use aes_gcm::aes::Aes128;
use log::debug;
use crate::network::arp_table::ArpTable;
use crate::network::blacklist::BlackList;
use crate::network::device_status::DeviceStatus;
use crate::network::device_status::DeviceStatus::Unknown;
use crate::network::packet::LoRaPacket;
use crate::network::queue::Queue;

pub struct ReceiveQueue<PacketT>
where PacketT: Into<PacketT> + TryFrom<PacketT> + Clone,
{
    queue: VecDeque<PacketT>,
}

impl ReceiveQueue<LoRaPacket>
{
    pub fn new() -> Self {
        ReceiveQueue {
            queue: VecDeque::new()
        }
    }

    pub fn packet_dispatch_thread(
        queue: Arc<Mutex<Self>>,
        arp_table: Arc<Mutex<ArpTable<u8, LoRaPacket, AesGcm<Aes128, U12>, U12>>>,
        blacklist: Arc<Mutex<BlackList<u8>>>,
    ) {
        let mut queue = Arc::clone(&queue);
        let mut arp_table = Arc::clone(&arp_table);
        let blacklist = Arc::clone(&blacklist);
        let packet_dispatch_thread_handle = thread::spawn(move || {
            loop {
                if !queue.lock().unwrap().queue.is_empty() {
                    let packet = queue.lock().unwrap().queue.pop_front().unwrap();
                    let blacklist = blacklist.lock().unwrap();
                    if blacklist.is_blacklisted(packet.header.source_addr) {
                        debug!("Received packet from blacklisted device! IGNORING PACKET");
                        continue;
                    }

                    let mut arp_table = arp_table.lock().unwrap();
                    if let Some(device) = arp_table.get_device(packet.header.source_addr) {
                        // add packet to device's packet_rx_vec
                        device.packet_rx_vec.push(packet.clone());
                        if packet.header.message_packet_num == packet.header.total_number_of_packets - 1 {
                            todo!("send signal to message assembler to assemble message");
                        }
                    } else {
                        debug!("Received packet from unknown device!");
                        // add packet to unknown device's packet_rx_vec
                        arp_table.add_device(packet.header.source_addr, DeviceStatus::Unknown);
                        let device = arp_table.get_device(packet.header.source_addr);
                        if let Some(device) = device {
                            device.packet_rx_vec.push(packet.clone());
                        } else {
                            panic!("Device was not added to arp table!");
                        }
                    }

                }
            }
        });
    }
}

impl <PacketT> Queue<PacketT> for ReceiveQueue<PacketT>
where PacketT: Into<PacketT> + TryFrom<PacketT> + Clone,
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

