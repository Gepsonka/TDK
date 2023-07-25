use std::collections::LinkedList;
use std::hash::Hash;
use std::sync::{Arc, Mutex};
use std::thread;
use aes_gcm::aead::generic_array::ArrayLength;
use aes_gcm::aes::cipher::crypto_common::InnerUser;
use aes_gcm::KeySizeUser;
use log::debug;
use crate::network::arp_table::ArpTable;
use crate::network::blacklist::BlackList;
use crate::network::packet::{LoRaPacket};
use crate::network::queue::Queue;

pub struct ReceiveQueue<PacketT, KeySize, NonceSize>
where PacketT: Into<PacketT> + TryFrom<PacketT>,
      KeySize: KeySizeUser + InnerUser
      NonceSize: ArrayLength<u8> + Clone + Copy + Eq + Hash
{
    queue: Vec<PacketT>,
}

impl <KeySize, NonceSize> ReceiveQueue<LoRaPacket, KeySize, NonceSize>
where KeySize: KeySizeUser + InnerUser,
      NonceSize: ArrayLength<u8> + Clone + Copy + Eq + Hash
{
    pub fn new() -> Self {
        ReceiveQueue {
            queue: Vec::new()
        }
    }

    pub fn packet_dispatch_thread(
        queue: &mut Self,
        arp_table: Arc<Mutex<ArpTable<u8, LoRaPacket, KeySize, NonceSize>>>,
        blacklist: Arc<Mutex<BlackList<u8>>>,
    ) {
        let mut arp_table = Arc::clone(&arp_table);
        let blacklist = Arc::clone(&blacklist);

        let packet_dispatch_thread_handle = thread::spawn(move || {
            loop {
                if !queue.is_empty() {
                    let packet = queue.pop().unwrap();
                    let blacklist = blacklist.lock().unwrap();
                    if *blacklist.is_blacklisted(packet.get_source_address()) {
                        debug!("Received packet from blacklisted device!");
                        continue;
                    }

                    match arp_table.get_device(packet.source_address) {
                        Some(device) => {
                            device.packet_received(packet);
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

impl <PacketT, KeySize, NonceSize> Queue<PacketT> for ReceiveQueue<PacketT, KeySize, NonceSize> {
    fn get_top_item(&mut self) -> Option<PacketT> {
        self.queue.front().clone()
    }

    fn pop(&mut self) -> Option<PacketT> {
        self.queue.pop_front()
    }

    fn push(&mut self, item: PacketT) {
        self.queue.push_back(item);
    }

    fn is_empty(&mut self) -> bool {
        self.queue.is_empty()
    }

    fn len(&mut self) -> usize {
        self.queue.len()
    }
}

