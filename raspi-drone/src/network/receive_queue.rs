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
use crate::network::packet::LoRaPacket;
use crate::network::queue::Queue;

pub struct ReceiveQueue<PacketT>
where PacketT: Into<PacketT> + TryFrom<PacketT> + Clone,
{
    pub queue: VecDeque<PacketT>,
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
                        debug!("Received packet from blacklisted device!");
                        continue;
                    }

                    let mut arp_table = arp_table.lock().unwrap();
                    match arp_table.get_device(packet.header.source_addr) {
                        Some(device) => {
                            device.packet_rx_vec.push(packet);
                        },
                        None => {
                            println!("Received packet from unknown device!");
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

