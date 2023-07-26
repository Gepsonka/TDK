use std::collections::HashMap;
use std::hash::Hash;

use aes_gcm::{Aes128Gcm, Aes256Gcm, AesGcm, Key, KeyInit, KeySizeUser};
use aes_gcm::aead::consts::U12;
use aes_gcm::aead::generic_array::ArrayLength;
use aes_gcm::aead::OsRng;
use aes_gcm::aes::Aes128;
use crate::network::arp_registry::{ArpRegistry, DeviceStatus};
use crate::network::packet::{LoRaPacket};

pub struct ArpTable<AddressSize, PacketT, KeySize, NonceSize>
    where KeySize: ArrayLength<u8> + 'static
{
    pub arp_table: HashMap<AddressSize, ArpRegistry<PacketT, KeySize, NonceSize>>
}


impl ArpTable<u8, LoRaPacket, AesGcm<Aes128, U12>, AesGcm<Aes128, U12>>
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

    pub fn get_device(&mut self, address: u8) -> Option<&mut ArpRegistry<LoRaPacket, AesGcm<Aes128, U12>, AesGcm<Aes128, U12>>> {
        self.arp_table.get_mut(&address)
    }

    pub fn get_device_status(&mut self, address: u8) -> Option<DeviceStatus> {
        match self.arp_table.get(&address) {
            Some(device) => Some(device.device_status),
            None => None
        }
    }

    pub fn get_device_secret_key(&mut self, address: u8) -> Option<Key<AesGcm<Aes128, U12>>> {
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

    pub fn is_address_in_table(&self, address: u8) -> bool {
        self.arp_table.contains_key(&address)
    }
}
