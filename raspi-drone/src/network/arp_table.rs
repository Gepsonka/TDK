use std::{collections::HashMap, hash::Hash};

use super::{arp_registry::ArpRegistry, packet::LoRaPacket};
use crate::network::arp_registry::RegistryFields;
use aes_gcm::Aes128Gcm;

pub trait ArpTableInit<AddressSize, ArpReg> {
    fn new() -> ArpTable<AddressSize, ArpReg>;
    fn new_from_registries(registries: Vec<ArpReg>) -> ArpTable<AddressSize, ArpReg>;
}

pub struct ArpTable<AddressSize, ArpReg> {
    pub table: HashMap<AddressSize, ArpReg>,
}

impl<AddressSize, ArpReg> ArpTable<AddressSize, ArpReg>
where
    AddressSize: Eq + PartialEq + Hash + Copy,
{
    pub fn get_registry(&self, address: AddressSize) -> Option<&ArpReg> {
        self.table.get(&address)
    }

    pub fn get_registry_mut(&mut self, address: AddressSize) -> Option<&mut ArpReg> {
        self.table.get_mut(&address)
    }

    pub fn insert_registry(&mut self, address: AddressSize, registry: ArpReg) {
        self.table.insert(address, registry);
    }
}

impl<AddressSize, ArpReg> ArpTableInit<AddressSize, ArpReg> for ArpTable<AddressSize, ArpReg>
where
    AddressSize: Eq + PartialEq + Hash + Copy,
    ArpReg: RegistryFields<AddressSize = AddressSize>,
{
    fn new() -> Self {
        Self {
            table: HashMap::new(),
        }
    }

    fn new_from_registries(registries: Vec<ArpReg>) -> Self {
        let mut arp_table = Self {
            table: HashMap::new(),
        };

        for registry in registries {
            if let Some(addr) = registry.get_address() {
                arp_table.table.insert(addr, registry);
            }
        }

        arp_table
    }
}
