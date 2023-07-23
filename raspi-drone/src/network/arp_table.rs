use std::collections::HashMap;
use std::hash::Hash;
use aes_gcm::aead::generic_array::ArrayLength;
use aes_gcm::{Aes256Gcm, Key, KeyInit, KeySizeUser};
use aes_gcm::aead::OsRng;
use crate::network::arp_registry::{ArpRegistry, DeviceStatus};
use crate::network::packet::Packet;

pub struct ArpTable<PacketT, KeySize, NonceSize>
where PacketT: Packet + Into<Vec<u8>> + From<Vec<u8>>,
      KeySize: KeySizeUser,
      NonceSize: ArrayLength<u8> + Clone + Copy + Eq + Hash
{
    pub arp_table: HashMap<u8, ArpRegistry<PacketT, KeySize, NonceSize>>
}


impl <PacketT, KeySize, NonceSize> ArpTable<PacketT, KeySize, NonceSize>
    where PacketT: Packet + Into<Vec<u8>> + From<Vec<u8>>,
          KeySize: KeySizeUser,
          NonceSize: ArrayLength<u8> + Clone + Copy + Eq + Hash
{
    pub fn new() -> Self {
        ArpTable {
            arp_table: HashMap::new()
        }
    }

    pub fn add_device(&mut self, address: u8, device_status: DeviceStatus) {
        self.arp_table.insert(address, ArpRegistry::new(address, device_status));
    }

    pub fn remove_device(&mut self, address: u8) {
        self.arp_table.remove(&address);
    }

    pub fn get_device(&mut self, address: u8) -> Option<&mut ArpRegistry<PacketT, KeySize, NonceSize>> {
        self.arp_table.get_mut(&address)
    }

    pub fn get_device_status(&mut self, address: u8) -> Option<DeviceStatus> {
        let key = Aes256Gcm::generate_key(&mut OsRng);
        match self.arp_table.get(&address) {
            Some(device) => Some(device.device_status),
            None => None
        }
    }

    pub fn get_device_secret_key(&mut self, address: u8) -> Option<Key<KeySize>> {
        let curr_device;
        match self.arp_table.get(&address) {
            Some(device) => curr_device = device,
            None => { return None; }
        };

        match &curr_device.secret_key {
            Some(secret_key) => Some(secret_key.clone()),
            None => None
        }
    }
}
